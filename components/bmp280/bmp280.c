#include "bmp280.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <math.h>

#define BMP280_I2C_ADDR         0x77
#define BMP280_I2C_PORT         I2C_NUM_0
#define BMP280_TIMEOUT_MS       1000

#define BMP280_REG_ID           0xD0
#define BMP280_REG_RESET        0xE0
#define BMP280_REG_STATUS       0xF3
#define BMP280_REG_CTRL         0xF4
#define BMP280_REG_CONFIG       0xF5
#define BMP280_REG_PRESS_MSB    0xF7
#define BMP280_REG_TEMP_MSB     0xFA
#define BMP280_REG_CALIB        0x88

static const char *TAG = "BMP280";

static uint16_t dig_T1;
static int16_t dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

static int32_t t_fine;

// read
static esp_err_t bmp280_read_reg(uint8_t reg, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BMP280_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BMP280_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(BMP280_I2C_PORT, cmd, pdMS_TO_TICKS(BMP280_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return err;
}

// write
static esp_err_t bmp280_write_reg(uint8_t reg, uint8_t value) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BMP280_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(BMP280_I2C_PORT, cmd, pdMS_TO_TICKS(BMP280_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return err;
}

static void bmp280_parse_calibration(uint8_t *data) {
    dig_T1 = (data[1] << 8) | data[0];
    dig_T2 = (data[3] << 8) | data[2];
    dig_T3 = (data[5] << 8) | data[4];
    dig_P1 = (data[7] << 8) | data[6];
    dig_P2 = (data[9] << 8) | data[8];
    dig_P3 = (data[11] << 8) | data[10];
    dig_P4 = (data[13] << 8) | data[12];
    dig_P5 = (data[15] << 8) | data[14];
    dig_P6 = (data[17] << 8) | data[16];
    dig_P7 = (data[19] << 8) | data[18];
    dig_P8 = (data[21] << 8) | data[20];
    dig_P9 = (data[23] << 8) | data[22];
}

static int32_t bmp280_compensate_temp(int32_t adc_T) {
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) *
                      ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) *
                    ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    return (t_fine * 5 + 128) / 256;
}

static uint32_t bmp280_compensate_pressure(int32_t adc_P) {
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) +
           ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;
    if (var1 == 0) return 0; // to avoid exception?
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;
    return ((p + var1 + var2) >> 8);
}

esp_err_t bmp280_init(void) {
    esp_log_level_set("*", ESP_LOG_INFO); 
    uint8_t chip_id = 0;
    ESP_ERROR_CHECK(bmp280_read_reg(BMP280_REG_ID, &chip_id, 1));
    if (chip_id != 0x58) {
        ESP_LOGE(TAG, "Unexpected chip ID: 0x%02X", chip_id);
        return ESP_FAIL;
    }

    uint8_t calib[24];
    ESP_ERROR_CHECK(bmp280_read_reg(BMP280_REG_CALIB, calib, 24));
    bmp280_parse_calibration(calib);

    // set config: standby 0.5ms, filter off
    ESP_ERROR_CHECK(bmp280_write_reg(BMP280_REG_CONFIG, 0x00));

    // ctrl_meas: temp oversampling x1, press x1, normal mode
    ESP_ERROR_CHECK(bmp280_write_reg(BMP280_REG_CTRL, 0x27));

    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "BMP280 init OK (chip ID 0x%02X)", chip_id);
    return ESP_OK;
}

esp_err_t bmp280_read(float *temperature, float *pressure) {
    uint8_t data[6];
    ESP_ERROR_CHECK(bmp280_read_reg(BMP280_REG_PRESS_MSB, data, 6));

    int32_t adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (data[2] >> 4);
    int32_t adc_T = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | (data[5] >> 4);

    int32_t temp_raw = bmp280_compensate_temp(adc_T);
    uint32_t press_raw = bmp280_compensate_pressure(adc_P);

    *temperature = temp_raw / 100.0f;   // convert to celsius

    float pressure_pa = press_raw / 256.0f;   // convert from Q24.8 to Pa
    *pressure = pressure_pa / 100.0f;         // Pa to hPa
    return ESP_OK;
}