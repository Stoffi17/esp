/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

static void blink_led(void)
{
    /* If the addressable LED is enabled */
    if (s_led_state) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, 16, 16, 16);
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    } else {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
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

#elif CONFIG_BLINK_LED_GPIO

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif

void configure_buttons(void)
{
    // GPIO-Konfiguration für Button 1
    gpio_config_t gpioConfigBtn1 = {
        .pin_bit_mask = (1ULL << CONFIG_BTN1_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&gpioConfigBtn1);

    // GPIO-Konfiguration für Button 2
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

    /* Configure the peripheral according to the LED type */
    configure_led();
    configure_buttons();

    uint32_t btn1Level = 1;
    uint32_t btn2Level = 1;

    while (1) {

        int currentBtn1Level = gpio_get_level(CONFIG_BTN1_GPIO);
        int currentBtn2Level = gpio_get_level(CONFIG_BTN2_GPIO);

        if (currentBtn1Level == 0 && btn1Level == 1) { // Button gedrückt (von HIGH zu LOW)
            ESP_LOGI(TAG, "Button 1 pressed!");
            ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
            blink_led();
            /* Toggle the LED state */
            s_led_state = !s_led_state;
        }

        if (currentBtn2Level == 0 && btn2Level == 1) { // Button gedrückt (von HIGH zu LOW)
            ESP_LOGI(TAG, "Button 2 pressed!");
            ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
            blink_led();
            /* Toggle the LED state */
            s_led_state = !s_led_state;
        }
        
        btn1Level = currentBtn1Level;
        btn2Level = currentBtn2Level;

        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
