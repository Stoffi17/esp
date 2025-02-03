#include <stdio.h>
//#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
//#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"
#include "iot_servo.h"

#include "webserver.h"

static const char *TAG = "rfid_project";

/* === RFID Reader Config === */
#define RC522_SPI_BUS_GPIO_MISO    (19)
#define RC522_SPI_BUS_GPIO_MOSI    (23)
#define RC522_SPI_BUS_GPIO_SCLK    (18)
#define RC522_SPI_SCANNER_GPIO_SDA (21)
#define RC522_SCANNER_GPIO_RST     (-1) // soft-reset

static rc522_spi_config_t driver_config = {
    .host_id = SPI3_HOST,
    .bus_config = &(spi_bus_config_t){
        .miso_io_num = RC522_SPI_BUS_GPIO_MISO,
        .mosi_io_num = RC522_SPI_BUS_GPIO_MOSI,
        .sclk_io_num = RC522_SPI_BUS_GPIO_SCLK,
    },
    .dev_config = {
        .spics_io_num = RC522_SPI_SCANNER_GPIO_SDA,
    },
    .rst_io_num = RC522_SCANNER_GPIO_RST,
};

static rc522_driver_handle_t driver;
static rc522_handle_t scanner;

typedef struct {
    uint8_t uid[RC522_PICC_UID_SIZE_MAX];
    uint8_t length;
} valid_card_t;

static valid_card_t *valid_cards = NULL;
static size_t valid_cards_count = 0;
static rc522_picc_uid_t current_uid;


/* === Servo Config === */
#define SERVO_GPIO          (13)  // Servo GPIO

static uint16_t calibration_value_0 = 0;    // Real 0 degree angle
static uint16_t calibration_value_180 = 180; // Real 180 degree angle

static void servo_init(void)
{
    ESP_LOGI(TAG, "Servo Control");

    // Configure the servo
    servo_config_t servo_cfg = {
        .max_angle = 180,
        .min_width_us = 500,
        .max_width_us = 2500,
        .freq = 50,
        .timer_number = LEDC_TIMER_0,
        .channels = {
            .servo_pin = {
                SERVO_GPIO,
            },
            .ch = {
                LEDC_CHANNEL_0,
            },
        },
        .channel_number = 1,
    };

    // Initialize the servo
    iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_cfg);
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, calibration_value_0);
}


/* === Global Variables === */
volatile bool card_present = false;
volatile bool valid_card_present = false;
volatile bool servo_reset_pending = false;
volatile bool door_locked = false;
uint32_t servo_close_delay_ms = 1000;
uint16_t servo_closed_offset = 0;
uint16_t servo_opened_offset = 0;


// Servo Control
static void servo_open(void)
{
    ESP_LOGI(TAG, "Opening Servo");
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, calibration_value_180 - 90 + servo_opened_offset);
}

static void servo_close_task(void *args)
{
    ESP_LOGI(TAG, "Closing Servo");
    vTaskDelay(servo_close_delay_ms / portTICK_PERIOD_MS);

    if(!card_present)
    {
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, calibration_value_0 + servo_closed_offset);
    }
    servo_reset_pending = false;
    vTaskDelete(NULL);
}


// RFID Control
static bool is_valid_card(const rc522_picc_t *picc)
{
    for (size_t i = 0; i < valid_cards_count; i++)
    {
        if (picc->uid.length == valid_cards[i].length &&
            memcmp(picc->uid.value, valid_cards[i].uid, picc->uid.length) == 0)
        {
            return true;
        }
    }
    return false;
}


static void on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;

    if (picc->state == RC522_PICC_STATE_ACTIVE) 
    {
        rc522_picc_print(picc);
        card_present = true;
        memcpy(current_uid.value, picc->uid.value, picc->uid.length);
        current_uid.length = picc->uid.length;
        if (is_valid_card(picc)) 
        {
            ESP_LOGI(TAG, "Card is valid");
            valid_card_present = true;
            servo_open();
        } else 
        {
            ESP_LOGI(TAG, "Card is not valid");
        }
    }
    else if (picc->state == RC522_PICC_STATE_IDLE && event->old_state >= RC522_PICC_STATE_ACTIVE) 
    {
        ESP_LOGI(TAG, "Card has been removed");
        card_present = false;
        valid_card_present = false;
        if (!servo_reset_pending) 
        {
            servo_reset_pending = true;
            xTaskCreate(servo_close_task, "servo_close_task", 2048, NULL, 5, NULL);
        }
    }
}

void add_card(void)
{
    // check if card can be added
    if (!card_present)
    {
        ESP_LOGI(TAG, "No card available.");
        return;
    } else if (valid_card_present)
    {
        ESP_LOGI(TAG, "Card already valid.");
        return;
    }

    // change array size
    valid_card_t *new_array = realloc(valid_cards, (valid_cards_count + 1) * sizeof(valid_card_t));
    if (new_array == NULL) 
    {
        ESP_LOGE(TAG, "Memory allocation failed!");
        return;
    }
    valid_cards = new_array;

    // add card
    memcpy(valid_cards[valid_cards_count].uid, current_uid.value, current_uid.length);
    valid_cards[valid_cards_count].length = current_uid.length;
    valid_cards_count++;

    ESP_LOGI(TAG, "New card added successfully!");
}

static void init_valid_cards(void)
{
    static const uint8_t init_uid[] = { 0xE3, 0x59, 0x28, 0xF7 };
    size_t init_length = sizeof(init_uid);

    valid_cards = malloc(sizeof(valid_card_t));
    if (valid_cards == NULL)
    {
        ESP_LOGE(TAG, "Memory allocation failed!");
        return;
    }

    memcpy(valid_cards[0].uid, init_uid, init_length);
    valid_cards[0].length = init_length;
    valid_cards_count = 1;
}


void app_main()
{
    init_valid_cards();

    // Init Servo
    servo_init();

    // Init RFID Reader
    rc522_spi_create(&driver_config, &driver);
    rc522_driver_install(driver);

    rc522_config_t scanner_config = {
        .driver = driver,
    };

    rc522_create(&scanner_config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_PICC_STATE_CHANGED, on_picc_state_changed, NULL);
    rc522_start(scanner);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Starte den Webserver
    if (start_webserver() == ESP_OK) {
        ESP_LOGI("Main", "Webserver erfolgreich gestartet");
    } else {
        ESP_LOGE("Main", "Fehler beim Starten des Webservers");
    }
}
