#include "i2c_init.h"
#include "aht20.h"
#include "bmp280.h"

// cpp wrapper for OTA self test
#include "boot_self_test.h"

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void ota_mark_if_pending(void);
static bool ota_self_test(void);

static const char *TAG = "ENV"; 
static const char *BOOT_TAG = "BOOT";

void app_main(void) {

    ota_mark_if_pending();

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

static bool ota_self_test(void)
{
    return boot_self_test_run();
}

static void ota_mark_if_pending(void) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;

    esp_err_t err = esp_ota_get_state_partition(running, &state);

    if (err == ESP_OK) {
        ESP_LOGI(BOOT_TAG, "Running partition: %s state=%d", running->label, (int)state);
        if (state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGW(BOOT_TAG, "Image is pending verification: running self test...");
            if (ota_self_test()) {
                ESP_LOGI(BOOT_TAG, "Self test pass: marking image as valid");
                ESP_ERROR_CHECK(esp_ota_mark_app_valid_cancel_rollback());
            } else {
                ESP_LOGE(BOOT_TAG, "Self test failed: rolling back");
                ESP_ERROR_CHECK(esp_ota_mark_app_invalid_rollback_and_reboot());
            }
        }
    } else {
        ESP_LOGI(BOOT_TAG, "Booting from factory image, no OTA state yet");
    }
}