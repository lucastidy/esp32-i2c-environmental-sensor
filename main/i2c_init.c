#include "i2c_init.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_check.h"

#define TAG "I2C_BUS"

esp_err_t i2c_bus_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(I2C_MASTER_PORT, &conf), TAG, "param_config failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0), TAG, "driver_install failed");
    return ESP_OK;
}

bool i2c_bus_probe(uint8_t addr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ACK from device at 0x%02X", addr);
        return true;
    } else {
        ESP_LOGW(TAG, "No ACK from 0x%02X (%s)", addr, esp_err_to_name(ret));
        return false;
    }
}
