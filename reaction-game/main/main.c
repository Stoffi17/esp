#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "esp_random.h"

static const char *TAG = "reaction_game";
static led_strip_handle_t led_strip;

#define MAX_TIME 15

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

void set_leds_red()
{
    for (int i = 0; i < CONFIG_LED_AMOUNT; i++)
    {
        led_strip_set_pixel(led_strip, i, 64, 0, 0);
    }
    led_strip_refresh(led_strip);
}

void set_leds_green()
{
    for (int i = 0; i < CONFIG_LED_AMOUNT; i++)
    {
        led_strip_set_pixel(led_strip, i, 0, 64, 0);
    }
    led_strip_refresh(led_strip);
}

void play_reaction_game()
{
    ESP_LOGI(TAG, "Starting Reaction Game...");
    set_leds_red();

    // Zufällige Verzögerung, bevor die LEDs grün werden
    int delay_ms = 2000 + (esp_random() % 3000); // 2 bis 5 Sekunden
    vTaskDelay(pdMS_TO_TICKS(delay_ms));

    set_leds_green();

    // Warte auf den ersten Button-Press
    while (1) {
        int player1 = gpio_get_level(CONFIG_BUTTON1_GPIO);
        int player2 = gpio_get_level(CONFIG_BUTTON2_GPIO);

        if (player1 == 0) {
            ESP_LOGI(TAG, "Player 1 wins!");
            break;
        }

        if (player2 == 0) {
            ESP_LOGI(TAG, "Player 2 wins!");
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Polling-Intervall
    }

    // LEDs zurücksetzen
    vTaskDelay(pdMS_TO_TICKS(2000)); // Kurze Pause
    set_leds_red();
}

void app_main(void)
{
    configure_led();
    configure_buttons();

    while (1)
    {
        play_reaction_game();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}