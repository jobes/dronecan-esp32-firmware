#include "http_server.h"
#include "esp_log.h"
#include "project_config.h"

static const char *TAG = "HTTP_SRV";

static const char index_html[] =
    "<!DOCTYPE html>"
    "<html><head>"
    "<meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "<title>" DEVICE_NAME "</title>"
    "<style>"
    "body{font-family:sans-serif;margin:0;padding:2rem;background:#1a1a2e;color:#e0e0e0;}"
    "h1{color:#0fbcf9;}"
    ".card{background:#16213e;border-radius:8px;padding:1.5rem;max-width:480px;margin:auto;}"
    "p{line-height:1.6;}"
    "</style>"
    "</head><body>"
    "<div class=\"card\">"
    "<h1>DroneCAN Gateway</h1>"
    "<p>Device: " DEVICE_NAME "</p>"
    "<p>HW: " STR(MAJOR_HW_VERSION) "." STR(MINOR_HW_VERSION) " &mdash; SW: " STR(MAJOR_SW_VERSION) "." STR(MINOR_SW_VERSION) "</p>"
                                                                                                                              "<p>Status: <strong style=\"color:#0be881\">Running</strong></p>"
                                                                                                                              "</div>"
                                                                                                                              "</body></html>";

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html, sizeof(index_html) - 1);
    return ESP_OK;
}

static const httpd_uri_t root_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
};

httpd_handle_t http_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &root_uri);
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
