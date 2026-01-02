#include "aht20.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define TAG "AHT20"

static esp_err_t aht20_write(uint8_t addr, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(AHT20_I2C_PORT, cmd, pdMS_TO_TICKS(AHT20_I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

void aht20_soft_reset(void) {
    uint8_t cmd = 0xBA;
    aht20_write(AHT20_I2C_ADDRESS, &cmd, 1);
    vTaskDelay(pdMS_TO_TICKS(20));
}

esp_err_t aht20_init(void) {
    uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};
    esp_err_t ret = aht20_write(AHT20_I2C_ADDRESS, init_cmd, 3);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "AHT20 init failed: %s", esp_err_to_name(ret));
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    return ret;
}

static void decode_raw_data(uint8_t *raw, float *temperature, float *humidity) {
    uint32_t hum_raw = ((raw[1] << 16) | (raw[2] << 8) | raw[3]) >> 4;
    uint32_t temp_raw = ((raw[3] & 0x0F) << 16) | (raw[4] << 8) | raw[5];

    *humidity = ((float)hum_raw * 100.0f) / 1048576.0f;
    *temperature = ((float)temp_raw * 200.0f / 1048576.0f) - 50.0f;
}

bool aht20_read(float *temperature, float *humidity) {
    uint8_t trigger[3] = {0xAC, 0x33, 0x00};
    if (aht20_write(AHT20_I2C_ADDRESS, trigger, 3) != ESP_OK) {
        ESP_LOGE(TAG, "Trigger failed");
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(80));

    uint8_t raw[6];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (AHT20_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, raw, 6, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(AHT20_I2C_PORT, cmd, pdMS_TO_TICKS(AHT20_I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: %s", esp_err_to_name(ret));
        return false;
    }

    if ((raw[0] & 0x80) != 0) {
        ESP_LOGW(TAG, "AHT20 not ready");
        return false;
    }

    decode_raw_data(raw, temperature, humidity);
    return true;
}
