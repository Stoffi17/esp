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

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 25, // at least one LED on board
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

void app_main(void)
{

    /* Configure the peripheral according to the LED type */
    configure_led();

    led_strip_set_pixel(led_strip, 0, 0, 0, 0);
    led_strip_set_pixel(led_strip, 1, 63, 0, 0);
    led_strip_set_pixel(led_strip, 2, 63, 0, 0);
    led_strip_set_pixel(led_strip, 3, 63, 0, 0);
    led_strip_set_pixel(led_strip, 4, 0, 0, 0);

    led_strip_set_pixel(led_strip, 5, 63, 0, 0);
    led_strip_set_pixel(led_strip, 6, 63, 0, 0);
    led_strip_set_pixel(led_strip, 7, 13, 38, 63);
    led_strip_set_pixel(led_strip, 8, 13, 38, 63);
    led_strip_set_pixel(led_strip, 9, 13, 38, 63);

    led_strip_set_pixel(led_strip, 10, 63, 0, 0);
    led_strip_set_pixel(led_strip, 11, 63, 0, 0);
    led_strip_set_pixel(led_strip, 12, 13, 38, 63);
    led_strip_set_pixel(led_strip, 13, 13, 38, 63);
    led_strip_set_pixel(led_strip, 14, 13, 38, 63);

    led_strip_set_pixel(led_strip, 15, 63, 0, 0);
    led_strip_set_pixel(led_strip, 16, 63, 0, 0);
    led_strip_set_pixel(led_strip, 17, 63, 0, 0);
    led_strip_set_pixel(led_strip, 18, 63, 0, 0);
    led_strip_set_pixel(led_strip, 19, 0, 0, 0);
    
    led_strip_set_pixel(led_strip, 20, 0, 0, 0);
    led_strip_set_pixel(led_strip, 21, 63, 0, 0);
    led_strip_set_pixel(led_strip, 22, 0, 0, 0);
    led_strip_set_pixel(led_strip, 23, 63, 0, 0);
    led_strip_set_pixel(led_strip, 24, 0, 0, 0);

    for (int i = 0; i < 24; i++) {
        led_strip_set_pixel(led_strip, i, 255, 255, 255);
    }
    
    led_strip_refresh(led_strip);

    while (1) {
    }
}
