#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "eieruhr";
static led_strip_handle_t led_strip;

#define MAX_TIME 15
#define BRIGHTNESS 64

void configure_led(void);
void configure_buttons(void);

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

void display_set_minutes(int minutes) {
    
    for (int i = 0; i < 15; i++) {
        if (i < minutes) {
            led_strip_set_pixel(led_strip, i, 0, BRIGHTNESS, 0);
        } else {
            led_strip_set_pixel(led_strip, i, 0, 0, 0);
        }
    }
    led_strip_refresh(led_strip);
}

void update_led_display(int timer, int total_timer) {
    
    int remaining_min = timer / 600;
    int remaining_sec = (timer % 600) / 10;
    ESP_LOGI(TAG, "Time remaining: %d minutes, %d seconds", remaining_min, remaining_sec);

    for (int i = 4; i >= 0; i--) {
        bool bit = (remaining_min >> i) & 0x01;
        if (bit) {
            led_strip_set_pixel(led_strip, 4 - i, BRIGHTNESS, BRIGHTNESS, BRIGHTNESS);
        } else {
            led_strip_set_pixel(led_strip, 4 - i, 0, 0, 0);
        }
    }

    for (int i = 4; i >= 0; i--) {
        bool bit = (remaining_sec >> i) & 0x01;
        if (bit) {
            led_strip_set_pixel(led_strip, 9 - i, 0, 0, BRIGHTNESS);
        } else {
            led_strip_set_pixel(led_strip, 9 - i, 0, 0, 0);
        }
    }

    int progress = total_timer - timer;
    float progress_ratio = (float)progress / (float)total_timer;
    int red_intensity = (int)((1.0 - progress_ratio) * BRIGHTNESS);
    int green_intensity = (int)(progress_ratio * BRIGHTNESS);

    for (int i = 15; i < 25; i++) {
        led_strip_set_pixel(led_strip, i, red_intensity, green_intensity, 0);
    }

    led_strip_refresh(led_strip);
}

void blink_leds(void) {
    led_strip_clear(led_strip);

    for (int i = 0; i < 5; i++) {
        for (int j = 15; j < 25; j++) {
            led_strip_set_pixel(led_strip, j, 0, 16, 0);
        }
        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(500));

        led_strip_clear(led_strip);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    configure_led();
    configure_buttons();

    ESP_LOGI(TAG, "Waiting for input...");
    int counter = 0;

    display_set_minutes(counter + 1);

    while (1) {

        // increment minutes
        int btn1Level = gpio_get_level(CONFIG_BUTTON1_GPIO);
        if (btn1Level == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));  // Debounce delay
            if (gpio_get_level(CONFIG_BUTTON1_GPIO) == 0) {
                counter = (counter + 1) % MAX_TIME;
                ESP_LOGI(TAG, "Time set to %d minutes", counter + 1);
                display_set_minutes(counter + 1);
            }
        }

        // start timer
        int btn2Level = gpio_get_level(CONFIG_BUTTON2_GPIO);
        if (btn2Level == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));  // Debounce delay
            if (gpio_get_level(CONFIG_BUTTON2_GPIO) == 0) {
                ESP_LOGI(TAG, "Starting %d minutes timer", counter + 1);
                int timer = (counter + 1) * 60 * 10;
                int total_timer = timer;

                while (timer > 0) {
                    update_led_display(timer, total_timer);

                    vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 ms
                    timer--;
                }
                ESP_LOGI(TAG, "Timer finished");

                blink_leds();
                display_set_minutes(counter + 1);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}