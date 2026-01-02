#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define BMP280_I2C_ADDRESS        0x77
#define BMP280_I2C_PORT           I2C_NUM_0
#define BMP280_I2C_TIMEOUT_MS     1000

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t bmp280_init(void);
esp_err_t bmp280_read(float *temperature, float *pressure); // °C, Pa

#ifdef __cplusplus
}
#endif
