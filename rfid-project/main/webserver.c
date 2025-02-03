#include "webserver.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "tags.h"

static const char *TAG = "webserver";

static void uid_to_hexstr(const uint8_t *uid, uint8_t length, char *out_str, size_t out_str_size)
{
    size_t pos = 0;
    for (uint8_t i = 0; i < length && pos < out_str_size - 1; i++)
    {
        pos += snprintf(out_str + pos, out_str_size - pos, "%02X", uid[i]);
    }
    out_str[pos] = '\0';
}

static bool hexstr_to_bytes(const char *hex_str, uint8_t *buf, size_t buf_len, size_t *out_len) 
{
    size_t hex_len = strlen(hex_str);
    if (hex_len % 2 != 0 || hex_len / 2 > buf_len) 
    {
        return false;
    }
    for (size_t i = 0; i < hex_len / 2; i++) 
    {
        unsigned int byte;
        if (sscanf(hex_str + 2 * i, "%2x", &byte) != 1) 
        {
            return false;
        }
        buf[i] = (uint8_t)byte;
    }
    *out_len = hex_len / 2;
    return true;
}


// URI-Handlers
static esp_err_t root_handler(httpd_req_t *req)
{
    char html_content[2048];
    char lockButtonText[32];

    if (door_locked) {
        strcpy(lockButtonText, "Unlock Door");
    } else {
        strcpy(lockButtonText, "Lock Door");
    }

    int len = snprintf(html_content, sizeof(html_content),
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "  <meta charset='utf-8'>"
        "  <title>Cat Door Manager 3000</title>"
        "  <script>"
        "    function openDoor() {"
        "      fetch('/open')"
        "        .then(response => response.text())"
        "        .then(data => { console.log(data); });"
        "    }"
        "    function closeDoor() {"
        "      fetch('/close')"
        "        .then(response => response.text())"
        "        .then(data => { console.log(data); });"
        "    }"
        "    function toggleLock() {"
        "      fetch('/togglelock')"
        "        .then(response => response.text())"
        "        .then(data => {"
        "          console.log(data);"
        "          location.reload();"
        "        });"
        "    }"
        "  </script>"
        "</head>"
        "<body>"
        "  <h1>Cat Door Manager 3000</h1>"
        "  <button onclick='openDoor()'>Open Door</button>"
        "  <button onclick='closeDoor()'>Close Door</button>"
        "  <button onclick='toggleLock()'>%s</button>"
        "  <br>"
        "  <button onclick='window.location.href=\"/tags\"'>Tag Manager</button>"
        "</body>"
        "</html>", lockButtonText);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_content, len);
    return ESP_OK;
}

static esp_err_t open_door_handler(httpd_req_t *req)
{
    servo_open();
    httpd_resp_sendstr(req, "Door opened");
    return ESP_OK;
}

static esp_err_t close_door_handler(httpd_req_t *req)
{
    servo_close();
    httpd_resp_sendstr(req, "Door closed");
    return ESP_OK;
}

static esp_err_t toggle_lock_handler(httpd_req_t *req)
{
    toggle_lock();
    
    char resp_str[64];
    snprintf(resp_str, sizeof(resp_str), "Door is now %s", door_locked ? "Locked" : "Unlocked");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, resp_str, strlen(resp_str));

    return ESP_OK;
}

static esp_err_t tags_handler(httpd_req_t *req)
{
    char *buf = malloc(4096);
    if (buf == NULL) 
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }
    
    int len = snprintf(buf, 4096,
        "<!DOCTYPE html><html>"
        "<head>"
        "<meta charset='utf-8'><title>Valid Tags</title>"
        "<style>"
        "table { border-collapse: collapse; width: 100%%; }"
        "th, td { border: 1px solid #ddd; padding: 8px; }"
        "th { background-color: #f2f2f2; }"
        "</style>"
        "</head><body>"
        "<button onclick='window.location.href=\"/\"'>Home</button>"
        "<h1>Valid Tags</h1>"
        "<table>"
        "<tr><th>UID</th><th>Name</th><th>Action</th></tr>");
    
    // Create table for valid tags
    for (size_t i = 0; i < valid_tags_count; i++)
    {
        char uid_str[RC522_PICC_UID_SIZE_MAX * 2 + 1];
        uid_to_hexstr(valid_tags[i].uid, valid_tags[i].length, uid_str, sizeof(uid_str));
        len += snprintf(buf + len, 4096 - len, 
                        "<tr>"
                        "<td>%s</td>"
                        "<td>%s</td>"
                        "<td><a href=\"/removetag?uid=%s\">Remove</a></td>"
                        "</tr>", 
                        uid_str, valid_tags[i].name, uid_str);
        if (len >= 4096) 
        {  
            break;
        }
    }
    
    len += snprintf(buf + len, 4096 - len, "</table>");

    // Add form
    len += snprintf(buf + len, 4096 - len,
        "<h2>Add New Tag</h2>"
        "<form action=\"/addtag\" method=\"GET\">"
        "<label for=\"name\">Name:</label>"
        "<input type=\"text\" id=\"name\" name=\"name\" maxlength=\"31\">"
        "<input type=\"submit\" value=\"Add\">"
        "</form>"
    );
    
    len += snprintf(buf + len, 4096 - len, "</body></html>");
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, buf, len);
    
    free(buf);
    return ESP_OK;
}

static esp_err_t add_tag_handler(httpd_req_t *req)
{
    char query[128];
    char name[64];
    
    // Get Query-String
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        // Get name parameter
        if (httpd_query_key_value(query, "name", name, sizeof(name)) == ESP_OK) {
            // Call add_tag function
            esp_err_t err = add_tag(name);
            httpd_resp_set_type(req, "text/html");
            if (err == ESP_OK) {
                const char *html_resp =
                    "<html><body>"
                    "<p>Tag added successfully</p>"
                    "<button onclick='window.location.href=\"/tags\"'>Back</button>"
                    "</body></html>";
                httpd_resp_send(req, html_resp, strlen(html_resp));
            } else {
                char resp_msg[128];
                snprintf(resp_msg, sizeof(resp_msg),
                         "<html><body>"
                         "<p>Error adding tag: 0x%x</p>"
                         "<button onclick='window.location.href=\"/tags\"'>Back</button>"
                         "</body></html>", err);
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, resp_msg);
            }
            return ESP_OK;
        }
    }
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'name' parameter");
    return ESP_FAIL;
}

static esp_err_t remove_tag_handler(httpd_req_t *req)
{
    char query[128];
    char uid_str[64];

    // Get Query-String
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        // Get uid parameter
        if (httpd_query_key_value(query, "uid", uid_str, sizeof(uid_str)) == ESP_OK) {
            uint8_t uid[RC522_PICC_UID_SIZE_MAX];
            size_t uid_len;
            if (!hexstr_to_bytes(uid_str, uid, RC522_PICC_UID_SIZE_MAX, &uid_len)) {
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid UID format");
                return ESP_FAIL;
            }
            esp_err_t err = remove_tag(uid, uid_len);
            httpd_resp_set_type(req, "text/html");
            if (err == ESP_OK) {
                const char *html_resp =
                    "<html><body>"
                    "<p>Tag removed successfully</p>"
                    "<button onclick='window.location.href=\"/tags\"'>Back</button>"
                    "</body></html>";
                httpd_resp_send(req, html_resp, strlen(html_resp));
            } else {
                const char *html_resp =
                    "<html><body>"
                    "<p>Tag not found</p>"
                    "<button onclick='window.location.href=\"/tags\"'>Back</button>"
                    "</body></html>";
                httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, html_resp);
            }
            return ESP_OK;
        }
    }
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'uid' parameter");
    return ESP_FAIL;
}


// URIs
static const httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t open_door_uri = {
    .uri       = "/open",
    .method    = HTTP_GET,
    .handler   = open_door_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t close_door_uri = {
    .uri       = "/close",
    .method    = HTTP_GET,
    .handler   = close_door_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t toggle_lock_uri = {
    .uri       = "/togglelock",
    .method    = HTTP_GET,
    .handler   = toggle_lock_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t tags_uri = {
    .uri       = "/tags",
    .method    = HTTP_GET,
    .handler   = tags_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t add_tag_uri = {
    .uri       = "/addtag",
    .method    = HTTP_GET,
    .handler   = add_tag_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t remove_tag_uri = {
    .uri       = "/removetag",
    .method    = HTTP_GET,
    .handler   = remove_tag_handler,
    .user_ctx  = NULL
};

static httpd_handle_t server = NULL;

static esp_err_t wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

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
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &open_door_uri);
        httpd_register_uri_handler(server, &close_door_uri);
        httpd_register_uri_handler(server, &toggle_lock_uri);
        httpd_register_uri_handler(server, &tags_uri);
        httpd_register_uri_handler(server, &add_tag_uri);
        httpd_register_uri_handler(server, &remove_tag_uri);
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
