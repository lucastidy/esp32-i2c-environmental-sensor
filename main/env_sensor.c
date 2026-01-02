#include "i2c_init.h"
#include "aht20.h"
#include "bmp280.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ENV"; 

void app_main(void) {
    if (i2c_bus_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C MASTER");
        return;
    }

    // probing for sensors
    if (i2c_bus_probe(0x38)) {
        ESP_LOGI(TAG, "AHT20 sensor found at address 0x38");
    } else {
        ESP_LOGE(TAG, "AHT20 sensor not found at address 0x38");
    }
    if (i2c_bus_probe(0x77)) {
        ESP_LOGI(TAG, "BMP280 sensor found at address 0x77");
    } else {
        ESP_LOGE(TAG, "BMP280 sensor not found at address 0x77");
    }

    aht20_soft_reset();
    if (aht20_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize AHT20 sensor");
        return;
    }

    if (bmp280_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BMP280 sensor");
        return;
    }

    while (1) {
        float temp_aht, hum;
        float temp_bmp, press;

        if (aht20_read(&temp_aht, &hum)) {
            printf("AHT20: Temp = %.2f°C | Humidity = %.2f%%\n", temp_aht, hum);
        }

        if (bmp280_read(&temp_bmp, &press) == ESP_OK) {
            printf("BMP280: Temp = %.2f °C | Pressure = %.2f hPa\n", temp_bmp, press);
        } else {
            ESP_LOGE("BMP280", "Failed to read sensor data");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}