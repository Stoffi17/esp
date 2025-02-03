#include "webserver.h"

#include <string.h>

#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "esp_log.h"

static const char *TAG = "webserver";

// Beispiel-Handler, der "Hello World" als HTML-Seite zurückgibt.
static esp_err_t hello_get_handler(httpd_req_t *req)
{
    const char *resp_str = "<!DOCTYPE html>"
                           "<html>"
                           "<head><meta charset='utf-8'><title>Hello</title></head>"
                           "<body><h1>Hello World!</h1></body>"
                           "</html>";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

// Definition des URI-Handlers für die Root-URL "/"
static const httpd_uri_t hello = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    .user_ctx  = NULL
};

static httpd_handle_t server = NULL;

static esp_err_t wifi_init_softap(void)
{
    // Initialisiere TCP/IP und den Standard-Event-Loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Erstelle das Standard-WiFi-AP-Interface
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "ESP32-AP",
            .ssid_len = strlen("ESP32-AP"),
            .password = "12345678",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    if (strlen((const char *)wifi_config.ap.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP gestartet. SSID:%s Passwort:%s", wifi_config.ap.ssid, wifi_config.ap.password);
    ESP_LOGI(TAG, "Verbinde Dich mit dem Netzwerk und öffne http://192.168.4.1 in Deinem Browser.");
    return ESP_OK;
}

esp_err_t start_webserver(void)
{
    // Start WiFi
    wifi_init_softap();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_LOGI(TAG, "Starting webserver on port: '%d'", config.server_port);
    
    // Start HTTP Server
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &hello);
        return ESP_OK;
    }
    return ESP_FAIL;
}

void stop_webserver(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
}
