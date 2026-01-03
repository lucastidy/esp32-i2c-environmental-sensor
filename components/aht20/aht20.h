#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define AHT20_I2C_ADDRESS        0x38
#define AHT20_I2C_PORT           I2C_NUM_0  // use shared bus
#define AHT20_I2C_TIMEOUT_MS     1000

#ifdef __cplusplus
extern "C" {
#endif

void aht20_soft_reset(void);
esp_err_t aht20_init(void);
bool aht20_read(float *temperature, float *humidity);

#ifdef __cplusplus
}
#endif
