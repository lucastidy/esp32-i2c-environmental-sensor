#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"


// shared I2C config for sensors
#define I2C_MASTER_PORT           I2C_NUM_0
#define I2C_MASTER_SDA_IO         26
#define I2C_MASTER_SCL_IO         25
#define I2C_MASTER_FREQ_HZ        100000
#define I2C_MASTER_TIMEOUT_MS     1000

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t i2c_bus_init(void);
bool i2c_bus_probe(uint8_t addr);

#ifdef __cplusplus
}
#endif
