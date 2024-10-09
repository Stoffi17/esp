#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "snake";
static led_strip_handle_t led_strip;

void configure_led_strip(void)
{
    ESP_LOGI(TAG, "Configuring addressable LED strip...");
    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_LED_STRIP_GPIO,
        .max_leds = CONFIG_LED_COUNT,
    };

#if CONFIG_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

void configure_buttons(void)
{
    ESP_LOGI(TAG, "Configuring buttons...");
    gpio_config_t gpioConfigBtn1 = {
        .pin_bit_mask = (1ULL << CONFIG_BTN1_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&gpioConfigBtn1);

    gpio_config_t gpioConfigBtn2 = {
        .pin_bit_mask = (1ULL << CONFIG_BTN2_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&gpioConfigBtn2);
}

void app_main(void)
{
    // Konfigurationen initialisieren
    configure_buttons();
    configure_led_strip();

    uint32_t btn1Level = 1;
    uint32_t btn2Level = 1;
    bool led_toggle = false;

    while (true) {
        // Lese den Zustand der Buttons
        int currentBtn1Level = gpio_get_level(CONFIG_BTN1_GPIO);
        int currentBtn2Level = gpio_get_level(CONFIG_BTN2_GPIO);

        // Überprüfe, ob Button 1 gedrückt wurde
        if (currentBtn1Level == 0 && btn1Level == 1) { // Button gedrückt (von HIGH zu LOW)
            led_toggle = !led_toggle; // Umschalten des LED-Zustands

            // Setze alle LEDs auf Rot oder schalte sie aus
            for (int i = 0; i < CONFIG_LED_COUNT; i++) {
                if (led_toggle) {
                    led_strip_set_pixel(led_strip, i, 255, 0, 0); // Rot
                } else {
                    led_strip_set_pixel(led_strip, i, 0, 0, 0); // Aus
                }
            }
            led_strip_refresh(led_strip);

            btn1Level = currentBtn1Level; // Aktualisiere den Button-Status
        }

        // Überprüfe, ob Button 2 gedrückt wurde (wenn eine andere Aktion benötigt wird)
        if (currentBtn2Level == 0 && btn2Level == 1) { // Button gedrückt (von HIGH zu LOW)
            // Testweise alle LEDs auf Grün setzen
            for (int i = 0; i < CONFIG_LED_COUNT; i++) {
                led_strip_set_pixel(led_strip, i, 0, 255, 0); // Grün
            }
            led_strip_refresh(led_strip);

            btn2Level = currentBtn2Level; // Aktualisiere den Button-Status
        }

        // Aktualisiere den Button-Status am Ende der Schleife
        btn1Level = currentBtn1Level;
        btn2Level = currentBtn2Level;

        vTaskDelay(pdMS_TO_TICKS(100));  // Kurze Verzögerung, um Tastenentprellung zu berücksichtigen
    }
}
