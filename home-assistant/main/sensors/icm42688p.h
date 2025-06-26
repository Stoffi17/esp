#ifndef ICM42688P_H
#define ICM42688P_H

#include "esp_err.h"

void spi_icm_init(void);
void icm_init(void);
esp_err_t icm_read_accel_g(float *ax_g, float *ay_g, float *az_g);

#endif // ICM42688P_H