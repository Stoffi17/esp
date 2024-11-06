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

static const char *TAG = "snake_game";
static led_strip_handle_t led_strip;

#define GRID_SIZE 25
#define GRID_ROWS 5
#define GRID_COLS 5
#define INITIAL_SNAKE_LENGTH 2

typedef enum { UP, DOWN, LEFT, RIGHT } direction_t;

typedef struct {
    int body[GRID_SIZE];
    int length;
    direction_t direction;
} snake_t;

snake_t snake;
int food_position = -1;

void configure_led(void);
void configure_buttons(void);
void init_snake(void);
void place_food(void);
void update_leds(void);
void move_snake(void);
void change_direction(direction_t turn);
bool is_collision(int new_head);

void configure_led(void)
{
    ESP_LOGI(TAG, "Configuring LEDs...");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_BLINK_GPIO,
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

void init_snake(void)
{
    ESP_LOGI(TAG, "Initializing Snake...");
    snake.length = INITIAL_SNAKE_LENGTH;
    snake.direction = UP;

    int start_position = 12;
    for (int i = 0; i < snake.length; i++) {
        snake.body[i] = start_position + 5*i;
    }
}

void place_food(void)
{
    ESP_LOGI(TAG, "Placing Food...");
    do {
        food_position = rand() % GRID_SIZE;
    } while (food_position == snake.body[0]);
}

void update_leds(void)
{
    ESP_LOGI(TAG, "Updating LEDs...");
    led_strip_clear(led_strip);

    ESP_LOGI(TAG, "Updating Snake LEDs...");
    for (int i = 0; i < snake.length; i++) {
        led_strip_set_pixel(led_strip, snake.body[i], 0, 16, 0);
    }

    ESP_LOGI(TAG, "Updating Food LEDs...");
    if (food_position >= 0) {
        led_strip_set_pixel(led_strip, food_position, 16, 0, 0);
    }
    led_strip_refresh(led_strip);
}

bool is_collision(int new_head)
{
    ESP_LOGI(TAG, "Checking for Collision...");
    if (new_head < 0 || new_head >= GRID_SIZE) {
        return true;
    }
    
    for (int i = 0; i < snake.length; i++) {
        if (snake.body[i] == new_head) {
            return true;
        }
    }

    return false;
}

void move_snake(void)
{
    ESP_LOGI(TAG, "Moving Snake...");
    //calculate new head position
    int head_row = snake.body[0] / GRID_COLS;
    int head_col = snake.body[0] % GRID_COLS;

    switch (snake.direction) {
        case UP:    head_row--; break;
        case DOWN:  head_row++; break;
        case LEFT:  head_col--; break;
        case RIGHT: head_col++; break;
    }

    int new_head = head_row * GRID_COLS + head_col;

    // check for collision
    if (is_collision(new_head)) {
        ESP_LOGI(TAG, "Game Over! Collision detected.");
        return;
    }

    // move body
    for (int i = snake.length - 1; i > 0; i--) {
        snake.body[i] = snake.body[i - 1];
    }
    snake.body[0] = new_head;

    // check for food
    if (snake.body[0] == food_position) {
        if (snake.length < GRID_SIZE) {
            snake.length++;
        }
        place_food();
    }
}

void change_direction(direction_t turn)
{
    ESP_LOGI(TAG, "Changing Direction...");
    if (turn == LEFT) {
        switch (snake.direction) {
            case UP:    snake.direction = LEFT; break;
            case DOWN:  snake.direction = RIGHT; break;
            case LEFT:  snake.direction = DOWN; break;
            case RIGHT: snake.direction = UP; break;
        }
        return;
    }
    else if (turn == RIGHT) {
        switch (snake.direction) {
            case UP:    snake.direction = RIGHT; break;
            case DOWN:  snake.direction = LEFT; break;
            case LEFT:  snake.direction = UP; break;
            case RIGHT: snake.direction = DOWN; break;
        }
        return;
    }
}

void app_main(void)
{

    configure_led();
    configure_buttons();
    init_snake();
    place_food();

    while (1) {

        int btn1Level = gpio_get_level(CONFIG_BUTTON1_GPIO);
        int btn2Level = gpio_get_level(CONFIG_BUTTON2_GPIO);

        if (btn1Level == 0) {
            change_direction(LEFT);
        }
        if (btn2Level == 0) {
            change_direction(RIGHT);
        }

        move_snake();
        update_leds();

        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
