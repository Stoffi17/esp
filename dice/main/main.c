#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "esp_random.h"

static const char *TAG = "dice";
static led_strip_handle_t led_strip;

#define COLOR (int[]){0, 255, 0}
#define BRIGHTNESS 0.2

void configure_led(void)
{
    ESP_LOGI(TAG, "Configuring LEDs...");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_BLINK_GPIO,
        .max_leds = CONFIG_LED_AMOUNT,
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
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
    ESP_LOGI(TAG, "Configuring Buttons...");
    // GPIO-Konfiguration für Button 1
    gpio_config_t gpioConfigBtn1 = {
        .pin_bit_mask = (1 << CONFIG_BUTTON1_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&gpioConfigBtn1);

    // GPIO-Konfiguration für Button 2
    gpio_config_t gpioConfigBtn2 = {
        .pin_bit_mask = (1 << CONFIG_BUTTON2_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&gpioConfigBtn2);
}

void showNum(int num) {
    bool zero[25] = {
        0,0,0,0,0,
        0,0,1,0,0,
        0,1,1,1,0,
        0,0,1,0,0,
        0,0,0,0,0
    };
    bool one[25] = {
        0,0,1,0,0,
        0,1,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,1,1,1,0
    };
    bool two[25] = {
        0,1,1,0,0,
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        0,1,1,1,0
    };
    bool three[25] = {
        0,1,1,0,0,
        0,0,0,1,0,
        0,1,1,1,0,
        0,0,0,1,0,
        0,1,1,0,0
    };
    bool four[25] = {
        0,1,0,1,0,
        0,1,0,1,0,
        0,1,1,1,0,
        0,0,0,1,0,
        0,0,0,1,0
    };
    bool five[25] = {
        0,1,1,1,0,
        0,1,0,0,0,
        0,1,1,0,0,
        0,0,0,1,0,
        0,1,1,0,0
    };
    bool six[25] = {
        0,0,1,1,0,
        0,1,0,0,0,
        0,1,1,0,0,
        0,1,0,1,0,
        0,0,1,0,0
    };

    bool *roll;

    switch (num) {
        case 0: roll = one; break;
        case 1: roll = two; break;
        case 2: roll = three; break;
        case 3: roll = four; break;
        case 4: roll = five; break;
        case 5: roll = six; break;
        default: roll = zero; break;
    }

    led_strip_clear(led_strip);
    for (int i = 0; i < 25; i++) {
        if (roll[i]) {
            vTaskDelay(pdMS_TO_TICKS(10));
            led_strip_set_pixel(led_strip, i, COLOR[0] * BRIGHTNESS, COLOR[1] * BRIGHTNESS, COLOR[2] * BRIGHTNESS);
        }
    }

    led_strip_refresh(led_strip);
}

void app_main(void)
{
    configure_led();
    configure_buttons();

    while (1) {

        ESP_LOGI(TAG, "Waiting for Button Press...");
        bool start = false;
        while (!start) {
            int btn2Level = gpio_get_level(CONFIG_BUTTON2_GPIO);
            if (btn2Level == 0) {
                vTaskDelay(pdMS_TO_TICKS(50));  // Debounce delay
                if (gpio_get_level(CONFIG_BUTTON2_GPIO) == 0) {
                    start = true;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        int random_number = (esp_random() % 6);
        ESP_LOGI(TAG, "Rolled %d", random_number);
        showNum(random_number);
    }
}
