/*
    Beschleunigungssensor
*/

#include "icm42688p.h"
#include "driver/spi_master.h"


// SPI Config
#define PIN_NUM_MISO 7
#define PIN_NUM_MOSI 4
#define PIN_NUM_CLK  10
#define PIN_NUM_CS   1

spi_device_handle_t icm_handle;

void spi_icm_init(void) {
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000, // 1 MHz
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &icm_handle));
}

// SPI-Register lesen
uint8_t icm_read_register(uint8_t reg) {
    uint8_t tx[2] = { reg | 0x80, 0x00 };
    uint8_t rx[2];

    spi_transaction_t t = {
        .length = 8 * sizeof(tx),
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    ESP_ERROR_CHECK(spi_device_transmit(icm_handle, &t));
    return rx[1];
}

// SPI-Register schreiben
void icm_write_register(uint8_t reg, uint8_t val) {
    uint8_t tx[2] = { reg & 0x7F, val };
    spi_transaction_t t = {
        .length = 8 * sizeof(tx),
        .tx_buffer = tx,
    };
    ESP_ERROR_CHECK(spi_device_transmit(icm_handle, &t));
}

// Beschleunigungssensor
void icm_init() {
    icm_write_register(0x06, 0x01);  // Soft Reset
    vTaskDelay(pdMS_TO_TICKS(100));

    icm_write_register(0x11, 0x00);  // Exit standby
    vTaskDelay(pdMS_TO_TICKS(10));

    icm_write_register(0x4E, 0x0F);  // PWR_MGMT0: Accel on, Low Noise
    vTaskDelay(pdMS_TO_TICKS(10));

    icm_write_register(0x50, 0x03);  // ACCEL_CONFIG0: ±2g, 1kHz
}

// Beschleunigungssensor lesen
esp_err_t icm_read_accel_g(float *ax_g, float *ay_g, float *az_g) {
    uint8_t tx[7] = { 0x1F | 0x80 };
    uint8_t rx[7] = {0};

    spi_transaction_t t = {
        .length = 8 * sizeof(tx),
        .tx_buffer = tx,
        .rx_buffer = rx,
    };

    esp_err_t ret = spi_device_transmit(icm_handle, &t);
    if (ret != ESP_OK) {
        return ret;
    }

    int16_t ax_raw = (int16_t)((rx[2] << 8) | rx[1]);
    int16_t ay_raw = (int16_t)((rx[4] << 8) | rx[3]);
    int16_t az_raw = (int16_t)((rx[6] << 8) | rx[5]);

    const float scale = 1.0f / 16384.0f;

    *ax_g = ax_raw * scale;
    *ay_g = ay_raw * scale;
    *az_g = az_raw * scale;

    return ESP_OK;
}
