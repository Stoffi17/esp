#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

// sensors
#include "icm42688p.h"
#include "shtc3.h"

// mqtt
#include "wifi.h"
#include "mqtt.h"


static const char *TAG = "home_assistant_project";


// Tasks
void task_shtc3(void *pvParameters) {
    float temp, hum;
    while (1) {
        if (shtc3_read(&temp, &hum) == ESP_OK) {
            char payload[64];

            // Temperature
            snprintf(payload, sizeof(payload), "%.2f", temp);
            mqtt_publish("home/cke/sensors/temperature", payload);

            // Humidity
            snprintf(payload, sizeof(payload), "%.2f", hum);
            mqtt_publish("home/cke/sensors/humidity", payload);

            ESP_LOGI(TAG, "Temperature: %.2f °C, Humidity: %.2f%%", temp, hum);
        } else {
            ESP_LOGE(TAG, "Error reading SHTC3");
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Alle 5 Sekunden
    }
}

void task_icm(void *pvParameters) {
    float ax_g, ay_g, az_g;
    while (1) {
        if (icm_read_accel_g(&ax_g, &ay_g, &az_g) == ESP_OK) {
            char payload[64];

            snprintf(payload, sizeof(payload), "{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f}", ax_g, ay_g, az_g);
            mqtt_publish("home/cke/sensors/acceleration", payload);

            ESP_LOGI(TAG, "Accel X: %.3f g, Y: %.3f g, Z: %.3f g", ax_g, ay_g, az_g);
        } else {
            ESP_LOGE(TAG, "Error reading ICM42688P");
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Alle 1 Sekunden
    }
}


// Main
void app_main(void)
{
    // Init
    wifi_init_sta();
    esp_err_t mqtt_app_start();
    i2c_master_init();
    spi_icm_init();
    icm_init();

    // Tasks starten
    xTaskCreate(task_shtc3, "shtc3_task", 2048, NULL, 1, NULL);
    xTaskCreate(task_icm, "icm_task", 2048, NULL, 1, NULL);
}

