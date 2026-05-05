#include "http_server.h"
#include "esp_log.h"
#include "project_config.h"

static const char *TAG = "HTTP_SRV";

#include "dronecan_mini/dronecan_value_params.h"
#include "dronecan_node_monitor.h"
#include "dronecan_data_monitor.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include <string.h>
#include <stdlib.h>

static void url_decode(char *src)
{
  char *dst = src;
  while (*src)
  {
    if (*src == '%' && src[1] && src[2])
    {
      int hi = src[1];
      int lo = src[2];
      if (hi >= '0' && hi <= '9')
        hi -= '0';
      else if (hi >= 'A' && hi <= 'F')
        hi -= 'A' - 10;
      else if (hi >= 'a' && hi <= 'f')
        hi -= 'a' - 10;
      if (lo >= '0' && lo <= '9')
        lo -= '0';
      else if (lo >= 'A' && lo <= 'F')
        lo -= 'A' - 10;
      else if (lo >= 'a' && lo <= 'f')
        lo -= 'a' - 10;
      *dst++ = (char)((hi << 4) | lo);
      src += 3;
    }
    else if (*src == '+')
    {
      *dst++ = ' ';
      src++;
    }
    else
    {
      *dst++ = *src++;
    }
  }
  *dst = '\0';
}

extern const uint8_t template_html_start[] asm("_binary_template_html_start");
extern const uint8_t template_html_end[] asm("_binary_template_html_end");
extern const uint8_t status_html_start[] asm("_binary_status_html_start");
extern const uint8_t status_html_end[] asm("_binary_status_html_end");
extern const uint8_t system_html_start[] asm("_binary_system_html_start");
extern const uint8_t system_html_end[] asm("_binary_system_html_end");
extern const uint8_t ap_html_start[] asm("_binary_ap_html_start");
extern const uint8_t ap_html_end[] asm("_binary_ap_html_end");
extern const uint8_t sta_html_start[] asm("_binary_sta_html_start");
extern const uint8_t sta_html_end[] asm("_binary_sta_html_end");
extern const uint8_t nodes_html_start[] asm("_binary_nodes_html_start");
extern const uint8_t nodes_html_end[] asm("_binary_nodes_html_end");
extern const uint8_t monitor_html_start[] asm("_binary_monitor_html_start");
extern const uint8_t monitor_html_end[] asm("_binary_monitor_html_end");

extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t style_css_end[] asm("_binary_style_css_end");
extern const uint8_t nodes_css_start[] asm("_binary_nodes_css_start");
extern const uint8_t nodes_css_end[] asm("_binary_nodes_css_end");
extern const uint8_t common_js_start[] asm("_binary_common_js_start");
extern const uint8_t common_js_end[] asm("_binary_common_js_end");
extern const uint8_t nodes_js_start[] asm("_binary_nodes_js_start");
extern const uint8_t nodes_js_end[] asm("_binary_nodes_js_end");
extern const uint8_t monitor_js_start[] asm("_binary_monitor_js_start");
extern const uint8_t monitor_js_end[] asm("_binary_monitor_js_end");
extern const uint8_t monitor_css_start[] asm("_binary_monitor_css_start");
extern const uint8_t monitor_css_end[] asm("_binary_monitor_css_end");
extern const uint8_t status_js_start[] asm("_binary_status_js_start");
extern const uint8_t status_js_end[] asm("_binary_status_js_end");

extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");

static void send_with_replacements(httpd_req_t *req, const char *template, size_t len, const char **keys, const char **values, int count)
{
  const char *curr = template;
  const char *end = template + len;

  while (curr < end)
  {
    const char *next_token = NULL;
    int token_idx = -1;

    for (int i = 0; i < count; i++)
    {
      const char *p = strstr(curr, keys[i]);
      if (p && (next_token == NULL || p < next_token))
      {
        next_token = p;
        token_idx = i;
      }
    }

    if (next_token == NULL)
    {
      httpd_resp_send_chunk(req, curr, end - curr);
      break;
    }

    httpd_resp_send_chunk(req, curr, next_token - curr);
    httpd_resp_send_chunk(req, values[token_idx], strlen(values[token_idx]));
    curr = next_token + strlen(keys[token_idx]);
  }
}

static esp_err_t render_page(httpd_req_t *req, const char *active_page, const uint8_t *content_start, const uint8_t *content_end, const char **keys, const char **values, int count)
{
  const char *template = (const char *)template_html_start;

  char nav_html[1024] = "<nav>";
  const char *pages[] = {"Status", "Nodes", "Monitor", "System", "AP", "STA"};
  const char *uris[] = {"/", "/nodes", "/monitor", "/system", "/ap", "/sta"};
  for (int i = 0; i < (int)ARRAY_SIZE(pages); i++)
  {
    const char *active_class = (strcmp(pages[i], active_page) == 0) ? " class=\"active\"" : "";
    char item[128];
    snprintf(item, sizeof(item), "<a href=\"%s\"%s>%s</a>", uris[i], active_class, pages[i]);
    strcat(nav_html, item);
  }
  strcat(nav_html, "</nav>");

  const char *nav_token = "{{NAV}}";
  const char *content_token = "{{CONTENT}}";
  const char *p_nav = strstr(template, nav_token);
  const char *p_content = strstr(template, content_token);

  if (!p_nav || !p_content)
    return httpd_resp_send_500(req);

  httpd_resp_set_type(req, "text/html");

  httpd_resp_send_chunk(req, template, p_nav - template);
  httpd_resp_send_chunk(req, nav_html, strlen(nav_html));

  const char *after_nav = p_nav + strlen(nav_token);
  httpd_resp_send_chunk(req, after_nav, p_content - after_nav);

  send_with_replacements(req, (const char *)content_start, content_end - content_start - 1, keys, values, count);

  const char *after_content = p_content + strlen(content_token);
  httpd_resp_send_chunk(req, after_content, (const char *)template_html_end - after_content - 1);

  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

typedef struct
{
  const uint8_t *start;
  const uint8_t *end;
  const char *type;
  bool is_text;
} asset_ctx_t;

static esp_err_t asset_get_handler(httpd_req_t *req)
{
  asset_ctx_t *ctx = (asset_ctx_t *)req->user_ctx;
  httpd_resp_set_type(req, ctx->type);
  httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=31536000");
  size_t len = ctx->end - ctx->start;
  if (ctx->is_text && len > 0)
    len--;
  httpd_resp_send(req, (const char *)ctx->start, len);
  return ESP_OK;
}

static const asset_ctx_t ctx_style_css = {style_css_start, style_css_end, "text/css", true};
static const asset_ctx_t ctx_nodes_css = {nodes_css_start, nodes_css_end, "text/css", true};
static const asset_ctx_t ctx_common_js = {common_js_start, common_js_end, "application/javascript", true};
static const asset_ctx_t ctx_nodes_js = {nodes_js_start, nodes_js_end, "application/javascript", true};
static const asset_ctx_t ctx_monitor_js = {monitor_js_start, monitor_js_end, "application/javascript", true};
static const asset_ctx_t ctx_monitor_css = {monitor_css_start, monitor_css_end, "text/css", true};
static const asset_ctx_t ctx_status_js = {status_js_start, status_js_end, "application/javascript", true};
static const asset_ctx_t ctx_favicon = {favicon_ico_start, favicon_ico_end, "image/x-icon", false};

static esp_err_t root_get_handler(httpd_req_t *req)
{
  char ip_str[16] = "0.0.0.0";
  esp_netif_ip_info_t ip_info;

  // Try STA first
  esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0)
  {
    esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
  }
  else
  {
    // Fallback to AP
    netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK)
    {
      esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
    }
  }

  char mdns_str[64] = "None";
  union DeviceParameter *p_hostname = get_device_parameter("HOSTNAME");
  if (p_hostname)
  {
    snprintf(mdns_str, sizeof(mdns_str), "%s.local", p_hostname->String.value);
  }

  char uptime_str[16];
  snprintf(uptime_str, sizeof(uptime_str), "%lld", (long long)(esp_timer_get_time() / 1000000));

  const char *keys[] = {"{{HW_VER}}", "{{SW_VER}}", "{{IP_ADDR}}", "{{MDNS_ADDR}}", "{{UPTIME}}"};
  const char *values[] = {
      STR(MAJOR_HW_VERSION) "." STR(MINOR_HW_VERSION),
      STR(MAJOR_SW_VERSION) "." STR(MINOR_SW_VERSION),
      ip_str,
      mdns_str,
      uptime_str};
  return render_page(req, "Status", status_html_start, status_html_end, keys, values, 5);
}

static esp_err_t system_get_handler(httpd_req_t *req)
{
  union DeviceParameter *p_hostname = get_device_parameter("HOSTNAME");
  const char *keys[] = {"{{HOSTNAME}}"};
  const char *values[] = {p_hostname ? p_hostname->String.value : ""};
  return render_page(req, "System", system_html_start, system_html_end, keys, values, 1);
}

static esp_err_t ap_get_handler(httpd_req_t *req)
{
  union DeviceParameter *p_ap_ssid = get_device_parameter("AP_SSID");
  union DeviceParameter *p_ap_pass = get_device_parameter("AP_PASSWORD");
  const char *keys[] = {"{{AP_SSID}}", "{{AP_PASSWORD}}"};
  const char *values[] = {p_ap_ssid ? p_ap_ssid->String.value : "", p_ap_pass ? p_ap_pass->String.value : ""};
  return render_page(req, "AP", ap_html_start, ap_html_end, keys, values, 2);
}

static esp_err_t sta_get_handler(httpd_req_t *req)
{
  union DeviceParameter *p_sta_ssid = get_device_parameter("STA_SSID");
  union DeviceParameter *p_sta_pass = get_device_parameter("STA_PASSWORD");
  const char *keys[] = {"{{STA_SSID}}", "{{STA_PASSWORD}}"};
  const char *values[] = {p_sta_ssid ? p_sta_ssid->String.value : "", p_sta_pass ? p_sta_pass->String.value : ""};
  return render_page(req, "STA", sta_html_start, sta_html_end, keys, values, 2);
}

static esp_err_t nodes_get_handler(httpd_req_t *req)
{
  return render_page(req, "Nodes", nodes_html_start, nodes_html_end, NULL, NULL, 0);
}

static esp_err_t monitor_get_handler(httpd_req_t *req)
{
  return render_page(req, "Monitor", monitor_html_start, monitor_html_end, NULL, NULL, 0);
}

static esp_err_t api_nodes_get_handler(httpd_req_t *req)
{
  char *buf = malloc(4096);
  if (!buf)
    return httpd_resp_send_500(req);

  dronecan_node_monitor_get_json(buf, 4096);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, buf, strlen(buf));
  free(buf);
  return ESP_OK;
}

static esp_err_t api_data_get_handler(httpd_req_t *req)
{
  char query[256];
  uint16_t ids[16];
  uint64_t signatures[16] = {0};
  int count = 0;

  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
  {
    char ids_str[128];
    if (httpd_query_key_value(query, "ids", ids_str, sizeof(ids_str)) == ESP_OK)
    {
      char *token = strtok(ids_str, ",");
      while (token && count < 16)
      {
        ids[count++] = (uint16_t)atoi(token);
        token = strtok(NULL, ",");
      }
    }

    char sigs_str[192];
    if (httpd_query_key_value(query, "sigs", sigs_str, sizeof(sigs_str)) == ESP_OK)
    {
      int sig_count = 0;
      char *token = strtok(sigs_str, ",");
      while (token && sig_count < count)
      {
        signatures[sig_count++] = (uint64_t)strtoull(token, NULL, 0);
        token = strtok(NULL, ",");
      }
    }
  }

  char *buf = malloc(2048);
  if (!buf)
    return httpd_resp_send_500(req);

  dronecan_data_monitor_update_subscriptions(ids, signatures, count);
  dronecan_data_monitor_get_json(buf, 2048, ids, count);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, buf, strlen(buf));
  free(buf);
  return ESP_OK;
}

static esp_err_t api_status_get_handler(httpd_req_t *req)
{
  char buf[64];
  snprintf(buf, sizeof(buf), "{\"uptime\": %lld}", (long long)(esp_timer_get_time() / 1000000));
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, buf, strlen(buf));
  return ESP_OK;
}

static esp_err_t config_post_handler(httpd_req_t *req)
{
  char buf[1024];
  int ret, remaining = req->content_len;

  if (remaining >= sizeof(buf))
  {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content too long");
    return ESP_FAIL;
  }

  ret = httpd_req_recv(req, buf, remaining);
  if (ret <= 0)
    return ESP_FAIL;
  buf[ret] = '\0';

  char val[64];
  const char *params[] = {"HOSTNAME", "AP_SSID", "AP_PASSWORD", "STA_SSID",
                          "STA_PASSWORD"};
  bool changed = false;

  for (int i = 0; i < (int)ARRAY_SIZE(params); i++)
  {
    if (httpd_query_key_value(buf, params[i], val, sizeof(val)) == ESP_OK)
    {
      url_decode(val);
      union DeviceParameter *p = get_device_parameter((char *)params[i]);
      if (p)
      {
        // Check if empty (except STA_SSID and STA_PASSWORD)
        if (i < 3 && strlen(val) == 0)
        {
          continue; // Skip invalid empty values for required fields
        }
        if (strcmp(p->String.value, val) != 0)
        {
          free(p->String.value);
          p->String.value = strdup(val);
          changed = true;
        }
      }
    }
  }

  if (changed)
  {
    save_parameters_to_nvs();
  }

  httpd_resp_set_status(req, "303 See Other");
  httpd_resp_set_hdr(req, "Location", "/");
  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}

static esp_err_t restart_post_handler(httpd_req_t *req)
{
  httpd_resp_send(req, "Restarting...", HTTPD_RESP_USE_STRLEN);
  vTaskDelay(pdMS_TO_TICKS(500));
  esp_restart();
  return ESP_OK;
}

static const httpd_uri_t root_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
};

static const httpd_uri_t system_uri = {
    .uri = "/system",
    .method = HTTP_GET,
    .handler = system_get_handler,
};

static const httpd_uri_t ap_uri = {
    .uri = "/ap",
    .method = HTTP_GET,
    .handler = ap_get_handler,
};

static const httpd_uri_t sta_uri = {
    .uri = "/sta",
    .method = HTTP_GET,
    .handler = sta_get_handler,
};

static const httpd_uri_t config_uri = {
    .uri = "/config",
    .method = HTTP_POST,
    .handler = config_post_handler,
};

static const httpd_uri_t restart_uri = {
    .uri = "/restart",
    .method = HTTP_POST,
    .handler = restart_post_handler,
};

static const httpd_uri_t nodes_uri = {
    .uri = "/nodes",
    .method = HTTP_GET,
    .handler = nodes_get_handler,
};

static const httpd_uri_t monitor_uri = {
    .uri = "/monitor",
    .method = HTTP_GET,
    .handler = monitor_get_handler,
};

static const httpd_uri_t api_nodes_uri = {
    .uri = "/api/nodes",
    .method = HTTP_GET,
    .handler = api_nodes_get_handler,
};

static const httpd_uri_t api_data_uri = {
    .uri = "/api/data",
    .method = HTTP_GET,
    .handler = api_data_get_handler,
};

static const httpd_uri_t api_status_uri = {
    .uri = "/api/status",
    .method = HTTP_GET,
    .handler = api_status_get_handler,
};

static const httpd_uri_t style_css_uri = {
    .uri = "/style.css",
    .method = HTTP_GET,
    .handler = asset_get_handler,
    .user_ctx = (void *)&ctx_style_css};

static const httpd_uri_t nodes_css_uri = {
    .uri = "/nodes.css",
    .method = HTTP_GET,
    .handler = asset_get_handler,
    .user_ctx = (void *)&ctx_nodes_css};

static const httpd_uri_t common_js_uri = {
    .uri = "/common.js",
    .method = HTTP_GET,
    .handler = asset_get_handler,
    .user_ctx = (void *)&ctx_common_js};

static const httpd_uri_t nodes_js_uri = {
    .uri = "/nodes.js",
    .method = HTTP_GET,
    .handler = asset_get_handler,
    .user_ctx = (void *)&ctx_nodes_js};

static const httpd_uri_t monitor_js_uri = {
    .uri = "/monitor.js",
    .method = HTTP_GET,
    .handler = asset_get_handler,
    .user_ctx = (void *)&ctx_monitor_js};

static const httpd_uri_t monitor_css_uri = {
    .uri = "/monitor.css",
    .method = HTTP_GET,
    .handler = asset_get_handler,
    .user_ctx = (void *)&ctx_monitor_css};

static const httpd_uri_t status_js_uri = {
    .uri = "/status.js",
    .method = HTTP_GET,
    .handler = asset_get_handler,
    .user_ctx = (void *)&ctx_status_js};

static const httpd_uri_t favicon_uri = {
    .uri = "/favicon.ico",
    .method = HTTP_GET,
    .handler = asset_get_handler,
    .user_ctx = (void *)&ctx_favicon};

httpd_handle_t http_server_start(void)
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 32;
  config.lru_purge_enable = true;
  httpd_handle_t server = NULL;

  if (httpd_start(&server, &config) == ESP_OK)
  {
    httpd_register_uri_handler(server, &api_status_uri);
    httpd_register_uri_handler(server, &root_uri);
    httpd_register_uri_handler(server, &system_uri);
    httpd_register_uri_handler(server, &ap_uri);
    httpd_register_uri_handler(server, &sta_uri);
    httpd_register_uri_handler(server, &config_uri);
    httpd_register_uri_handler(server, &restart_uri);
    httpd_register_uri_handler(server, &nodes_uri);
    httpd_register_uri_handler(server, &monitor_uri);
    httpd_register_uri_handler(server, &api_nodes_uri);
    httpd_register_uri_handler(server, &api_data_uri);
    httpd_register_uri_handler(server, &style_css_uri);
    httpd_register_uri_handler(server, &nodes_css_uri);
    httpd_register_uri_handler(server, &common_js_uri);
    httpd_register_uri_handler(server, &nodes_js_uri);
    httpd_register_uri_handler(server, &monitor_js_uri);
    httpd_register_uri_handler(server, &monitor_css_uri);
    httpd_register_uri_handler(server, &status_js_uri);
    httpd_register_uri_handler(server, &favicon_uri);
    ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
  }
  else
  {
    ESP_LOGE(TAG, "Failed to start HTTP server");
  }

  return server;
}

void http_server_stop(httpd_handle_t server)
{
  if (server)
  {
    httpd_stop(server);
    ESP_LOGI(TAG, "HTTP server stopped");
  }
}
