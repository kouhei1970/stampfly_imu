/**
 * @file main.c
 * @brief BMI270 Stage 3: Polling Data Read Example
 *
 * This example demonstrates:
 * - BMI270 initialization
 * - Sensor configuration (range, ODR, filter)
 * - Continuous data reading using polling
 * - Display of both raw and physical values
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bmi270_spi.h"
#include "bmi270_init.h"
#include "bmi270_data.h"

static const char *TAG = "BMI270_STAGE3";

// M5StampFly BMI270 pin configuration
#define BMI270_MOSI_PIN     14
#define BMI270_MISO_PIN     43
#define BMI270_SCLK_PIN     44
#define BMI270_CS_PIN       46
#define BMI270_SPI_CLOCK_HZ 10000000  // 10 MHz
#define PMW3901_CS_PIN      12        // Other device on shared SPI bus

// Polling interval
#define POLLING_INTERVAL_MS 100  // 100ms = 10Hz

void app_main(void)
{
    esp_err_t ret;
    bmi270_dev_t dev = {0};

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " BMI270 Stage 3: Polling Data Read");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Step 1: Initialize SPI
    ESP_LOGI(TAG, "Step 1: Initializing SPI...");
    bmi270_config_t config = {
        .gpio_mosi = BMI270_MOSI_PIN,
        .gpio_miso = BMI270_MISO_PIN,
        .gpio_sclk = BMI270_SCLK_PIN,
        .gpio_cs = BMI270_CS_PIN,
        .spi_clock_hz = BMI270_SPI_CLOCK_HZ,
        .spi_host = SPI2_HOST,
        .gpio_other_cs = PMW3901_CS_PIN
    };

    ret = bmi270_spi_init(&dev, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ SPI initialization failed");
        return;
    }
    ESP_LOGI(TAG, "✓ SPI initialized successfully");
    ESP_LOGI(TAG, "");

    // Step 2: Activate SPI mode
    ESP_LOGI(TAG, "Step 2: Activating SPI mode...");
    uint8_t dummy;
    bmi270_read_register(&dev, BMI270_REG_CHIP_ID, &dummy);  // First dummy read
    vTaskDelay(pdMS_TO_TICKS(5));  // Wait 5ms
    bmi270_read_register(&dev, BMI270_REG_CHIP_ID, &dummy);  // Second dummy read
    ESP_LOGI(TAG, "SPI mode activated");
    ESP_LOGI(TAG, "");

    // Step 3: Initialize BMI270
    ESP_LOGI(TAG, "Step 3: Initializing BMI270...");
    ret = bmi270_init(&dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ BMI270 initialization failed");
        return;
    }
    ESP_LOGI(TAG, "✓ BMI270 initialized successfully");
    ESP_LOGI(TAG, "");

    // Step 4: Configure sensor settings
    ESP_LOGI(TAG, "Step 4: Configuring sensors...");

    // Set accelerometer to ±4g range
    ret = bmi270_set_accel_range(&dev, BMI270_ACC_RANGE_4G);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set accelerometer range");
    } else {
        ESP_LOGI(TAG, "Accelerometer range set to ±4g");
    }

    // Set gyroscope to ±1000 °/s range
    ret = bmi270_set_gyro_range(&dev, BMI270_GYR_RANGE_1000DPS);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set gyroscope range");
    } else {
        ESP_LOGI(TAG, "Gyroscope range set to ±1000 °/s");
    }

    // Set accelerometer to 100Hz, Performance mode
    ret = bmi270_set_accel_config(&dev, BMI270_ACC_ODR_100HZ, BMI270_FILTER_PERFORMANCE);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set accelerometer config");
    } else {
        ESP_LOGI(TAG, "Accelerometer configured: 100Hz, Performance mode");
    }

    // Set gyroscope to 200Hz, Performance mode
    ret = bmi270_set_gyro_config(&dev, BMI270_GYR_ODR_200HZ, BMI270_FILTER_PERFORMANCE);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set gyroscope config");
    } else {
        ESP_LOGI(TAG, "Gyroscope configured: 200Hz, Performance mode");
    }

    ESP_LOGI(TAG, "✓ Sensor configuration complete");
    ESP_LOGI(TAG, "");

    // Step 5: Start continuous data reading
    ESP_LOGI(TAG, "Step 5: Starting continuous data reading...");
    ESP_LOGI(TAG, "Polling interval: %d ms", POLLING_INTERVAL_MS);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " Data Stream (press Ctrl+] to stop)");
    ESP_LOGI(TAG, "========================================");

    vTaskDelay(pdMS_TO_TICKS(1000));  // Wait 1 second before starting

    uint32_t sample_count = 0;

    while (1) {
        // Read raw data
        bmi270_raw_data_t acc_raw, gyr_raw;
        ret = bmi270_read_accel_raw(&dev, &acc_raw);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read accelerometer");
            vTaskDelay(pdMS_TO_TICKS(POLLING_INTERVAL_MS));
            continue;
        }

        ret = bmi270_read_gyro_raw(&dev, &gyr_raw);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read gyroscope");
            vTaskDelay(pdMS_TO_TICKS(POLLING_INTERVAL_MS));
            continue;
        }

        // Read physical values
        bmi270_accel_t accel;
        bmi270_gyro_t gyro;
        ret = bmi270_read_accel(&dev, &accel);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to convert accelerometer data");
            vTaskDelay(pdMS_TO_TICKS(POLLING_INTERVAL_MS));
            continue;
        }

        ret = bmi270_read_gyro(&dev, &gyro);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to convert gyroscope data");
            vTaskDelay(pdMS_TO_TICKS(POLLING_INTERVAL_MS));
            continue;
        }

        // Read temperature
        float temperature;
        ret = bmi270_read_temperature(&dev, &temperature);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to read temperature");
            temperature = 0.0f;  // Set to 0 if read fails
        }

        // Display data
        sample_count++;

        printf("\n[Sample #%lu]\n", sample_count);
        printf("  Accelerometer (±4g):\n");
        printf("    Raw:      X=%6d  Y=%6d  Z=%6d [LSB]\n",
               acc_raw.x, acc_raw.y, acc_raw.z);
        printf("    Physical: X=%7.3f  Y=%7.3f  Z=%7.3f [g]\n",
               accel.x, accel.y, accel.z);

        printf("  Gyroscope (±1000 °/s):\n");
        printf("    Raw:      X=%6d  Y=%6d  Z=%6d [LSB]\n",
               gyr_raw.x, gyr_raw.y, gyr_raw.z);
        printf("    Physical: X=%8.2f  Y=%8.2f  Z=%8.2f [°/s]\n",
               gyro.x, gyro.y, gyro.z);

        printf("  Temperature:\n");
        printf("    Value:    %6.2f [°C]\n", temperature);

        // Wait for next sample
        vTaskDelay(pdMS_TO_TICKS(POLLING_INTERVAL_MS));
    }
}
