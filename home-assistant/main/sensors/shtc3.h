#ifndef SHTC3_H
#define SHTC3_H

#include "esp_err.h"

void i2c_master_init(void);
esp_err_t shtc3_read(float *temperature, float *humidity);

#endif // SHTC3_H