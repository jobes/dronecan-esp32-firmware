#include "wifi_manager.h"
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "dronecan_mini/dronecan_value_params.h"
#include "esp_mac.h"
#include "project_config.h"
#include "mdns.h"
#include "http_server.h"

static const char *TAG = "WIFI_MGR";

static int s_retry_num = 0;
static httpd_handle_t s_http_server = NULL;
#define MAXIMUM_RETRY 5

// ---- mDNS helpers ----------------------------------------------------------

static void mdns_start_service(void)
{
    union DeviceParameter *param = get_device_parameter("HOSTNAME");

    if (!param || !param->String.value || strlen(param->String.value) == 0)
    {
        mdns_free();
        ESP_LOGI(TAG, "mDNS disabled: HOSTNAME is empty");
        return;
    }

    const char *mdns_name = param->String.value;

    // Stop any existing mDNS instance before (re)starting
    mdns_free();

    esp_err_t err = mdns_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "mDNS init failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_ERROR_CHECK(mdns_hostname_set(mdns_name));
    ESP_ERROR_CHECK(mdns_instance_name_set(DEVICE_NAME));

    // Advertise an HTTP service so the device is easily discoverable
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);

    // Advertise Cannelloni (CAN-over-UDP) service for CAN bus tunneling
    mdns_service_add(NULL, "_cannelloni", "_udp", 20000, NULL, 0);

    ESP_LOGI(TAG, "mDNS started: hostname='%s.local', instance='%s'", mdns_name, DEVICE_NAME);
}

static void ensure_http_server(void)
{
    if (!s_http_server)
    {
        s_http_server = http_server_start();
    }
}

// ---------------------------------------------------------------------------

// Forward declarations
static void wifi_ap_fallback_start(void);
static void wifi_sta_start(void);

static TimerHandle_t s_reconnect_timer = NULL;

// Timer callback that tries to reconnect STA
static void reconnect_timer_cb(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "Reconnection timer expired. Retrying STA connection in background...");

    char *sta_ssid = get_device_parameter("STA_SSID")->String.value;
    char *sta_pass = get_device_parameter("STA_PASSWORD")->String.value;
    if (sta_ssid && strlen(sta_ssid) > 0)
    {
        wifi_config_t wifi_config = {
            .sta = {
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            },
        };
        strncpy((char *)wifi_config.sta.ssid, sta_ssid, sizeof(wifi_config.sta.ssid) - 1);
        strncpy((char *)wifi_config.sta.password, sta_pass ? sta_pass : "", sizeof(wifi_config.sta.password) - 1);

        if (!sta_pass || strlen(sta_pass) == 0)
        {
            wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
        }
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

        s_retry_num = 0;
        esp_wifi_connect();
    }
    else
    {
        ESP_LOGW(TAG, "STA_SSID is empty. Will check again in 10s.");
        xTimerStart(s_reconnect_timer, 0);
    }
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "Retry connection to AP (%d/%d)", s_retry_num, MAXIMUM_RETRY);
        }
        else
        {
            wifi_mode_t mode;
            esp_wifi_get_mode(&mode);
            if (mode == WIFI_MODE_APSTA || mode == WIFI_MODE_AP)
            {
                ESP_LOGW(TAG, "STA connection failed. Retrying again in 10s.");
                if (s_reconnect_timer)
                {
                    xTimerStart(s_reconnect_timer, 0);
                }
            }
            else
            {
                ESP_LOGE(TAG, "Failed to connect to STA after %d retries. Enabling fallback AP.", MAXIMUM_RETRY);
                wifi_ap_fallback_start();
            }
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got valid IP from STA: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0; // Reset retries on successful connection

        if (s_reconnect_timer)
        {
            xTimerStop(s_reconnect_timer, 0);
        }

        wifi_mode_t mode;
        esp_wifi_get_mode(&mode);
        if (mode == WIFI_MODE_APSTA || mode == WIFI_MODE_AP)
        {
            ESP_LOGI(TAG, "Successfully connected to STA. Disabling fallback AP.");
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        }

        mdns_start_service();
        ensure_http_server();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Station " MACSTR " joined AP, AID=%d", MAC2STR(event->mac), event->aid);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Station " MACSTR " left AP, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

static void wifi_ap_fallback_start(void)
{
    esp_wifi_stop();

    char *ap_ssid = get_device_parameter("AP_SSID")->String.value;
    char *ap_pass = get_device_parameter("AP_PASSWORD")->String.value;

    if (!ap_ssid || strlen(ap_ssid) == 0)
        ap_ssid = "airplane_gateway";
    if (!ap_pass || strlen(ap_pass) == 0)
        ap_pass = "";

    wifi_config_t wifi_config = {
        .ap = {
            .channel = 6,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    // Using string length protection. Mac length is 32 and 64 for SSID and PASSWORD inside wifi_config_t.
    strncpy((char *)wifi_config.ap.ssid, ap_ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);
    strncpy((char *)wifi_config.ap.password, ap_pass, sizeof(wifi_config.ap.password) - 1);

    if (strlen(ap_pass) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Fallback AP Started successfully. SSID: '%s'. Background STA retry active.", ap_ssid);

    mdns_start_service();
    ensure_http_server();

    // Start 10s timer to trigger reconnect without stopping AP
    if (s_reconnect_timer == NULL)
    {
        s_reconnect_timer = xTimerCreate("reconn_timer", pdMS_TO_TICKS(10000), pdFALSE, NULL, reconnect_timer_cb);
    }

    if (s_reconnect_timer)
    {
        xTimerStart(s_reconnect_timer, 0);
    }
}

static void wifi_sta_start(void)
{
    char *sta_ssid = get_device_parameter("STA_SSID")->String.value;
    char *sta_pass = get_device_parameter("STA_PASSWORD")->String.value;

    if (!sta_ssid || strlen(sta_ssid) == 0)
    {
        ESP_LOGW(TAG, "STA_SSID is empty, skipping STA and starting Fallback AP directly.");
        wifi_ap_fallback_start();
        return;
    }

    esp_wifi_stop();

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    // Proper copy avoiding buffer overflow
    strncpy((char *)wifi_config.sta.ssid, sta_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, sta_pass ? sta_pass : "", sizeof(wifi_config.sta.password) - 1);

    if (!sta_pass || strlen(sta_pass) == 0)
    {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    s_retry_num = 0;

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "STA start finished. Trying to connect to SSID: '%s'", sta_ssid);
}

void wifi_init_manager(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    union DeviceParameter *hostname_param = get_device_parameter("HOSTNAME");
    if (hostname_param && hostname_param->String.value && strlen(hostname_param->String.value) > 0)
    {
        esp_netif_set_hostname(sta_netif, hostname_param->String.value);
        ESP_LOGI(TAG, "Hostname set to: %s", hostname_param->String.value);
    }

    // Set DHCP Option 60 – Vendor Class Identifier
    // Identifies the device type/vendor to the DHCP server
    esp_err_t opt60_err = esp_netif_dhcpc_option(
        sta_netif,
        ESP_NETIF_OP_SET,
        ESP_NETIF_VENDOR_CLASS_IDENTIFIER,
        DEVICE_NAME,
        DEVICE_NAME_LEN);
    if (opt60_err != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to set DHCP Option 60 (Vendor Class Identifier): %s", esp_err_to_name(opt60_err));
    }
    else
    {
        ESP_LOGI(TAG, "DHCP Option 60 set to: %s", DEVICE_NAME);
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));

    wifi_sta_start();
}
