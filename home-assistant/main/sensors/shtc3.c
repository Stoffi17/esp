/*
    Beschleunigungssensor
*/

#include "shtc3.h"
#include "driver/i2c.h"


// I2C Config
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA_IO   5
#define I2C_MASTER_SCL_IO   6
#define I2C_MASTER_FREQ_HZ  100000
#define SHTC3_ADDR          0x70

void i2c_master_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// Temperatur- und Luftfeuchtigkeitssensor
esp_err_t shtc3_read(float *temperature, float *humidity) {
    esp_err_t ret;
    uint8_t data[6];

    // Measurement command
    uint8_t measure_cmd[2] = {0x78, 0x66};
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SHTC3_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, measure_cmd, 2, true);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) return ret;

    vTaskDelay(pdMS_TO_TICKS(20));  // Warte auf Messung

    // Read 6 bytes (T_MSB, T_LSB, T_CRC, RH_MSB, RH_LSB, RH_CRC)
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SHTC3_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 6, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) return ret;

    uint16_t temp_raw = (data[0] << 8) | data[1];
    uint16_t hum_raw  = (data[3] << 8) | data[4];

    *temperature = 175 * (float)temp_raw / 65536.0f - 45.0f;
    *humidity    = 100 * (float)hum_raw / 65536.0f;

    return ESP_OK;
}
