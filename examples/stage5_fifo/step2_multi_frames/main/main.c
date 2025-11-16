/**
 * @file main.c
 * @brief BMI270 Step 2: FIFO Multiple Frames Read
 *
 * This example demonstrates:
 * - Reading all available FIFO data in one burst
 * - Parsing multiple frames from FIFO buffer
 * - Handling special headers (0x40, 0x48)
 * - Preventing data loss by reading FIFO_LENGTH bytes
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "bmi270_spi.h"
#include "bmi270_init.h"
#include "bmi270_data.h"

static const char *TAG = "BMI270_STEP2";

// M5StampFly BMI270 pin configuration
#define BMI270_MOSI_PIN     14
#define BMI270_MISO_PIN     43
#define BMI270_SCLK_PIN     44
#define BMI270_CS_PIN       46
#define BMI270_SPI_CLOCK_HZ 10000000  // 10 MHz
#define PMW3901_CS_PIN      12        // Other device on shared SPI bus

// FIFO registers
#define BMI270_REG_FIFO_LENGTH_0    0x24    // FIFO length LSB
#define BMI270_REG_FIFO_LENGTH_1    0x25    // FIFO length MSB
#define BMI270_REG_FIFO_DATA        0x26    // FIFO data read
#define BMI270_REG_FIFO_CONFIG_0    0x48    // FIFO mode config
#define BMI270_REG_FIFO_CONFIG_1    0x49    // FIFO sensor enable

// FIFO constants
#define FIFO_FRAME_SIZE_HEADER      13      // Header(1) + GYR(6) + ACC(6)
#define FIFO_HEADER_ACC_GYR         0x8C    // Expected header for ACC+GYR frame
#define FIFO_HEADER_SKIP            0x40    // Skip frame (data loss)
#define FIFO_HEADER_CONFIG          0x48    // Config change frame
#define FIFO_MAX_SIZE               2048    // Maximum FIFO size

// Global device handle
static bmi270_dev_t g_dev = {0};

// Statistics
static uint32_t g_total_frames = 0;
static uint32_t g_valid_frames = 0;
static uint32_t g_skip_frames = 0;
static uint32_t g_config_frames = 0;

/**
 * @brief Read FIFO length
 */
static esp_err_t read_fifo_length(uint16_t *length)
{
    uint8_t length_data[2];
    esp_err_t ret = bmi270_read_burst(&g_dev, BMI270_REG_FIFO_LENGTH_0, length_data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    // FIFO length is 11-bit value (0-2047 bytes)
    *length = (uint16_t)((length_data[1] << 8) | length_data[0]) & 0x07FF;
    return ESP_OK;
}

/**
 * @brief Read multiple frames from FIFO
 */
static esp_err_t read_fifo_data(uint8_t *buffer, uint16_t length)
{
    return bmi270_read_burst(&g_dev, BMI270_REG_FIFO_DATA, buffer, length);
}

/**
 * @brief Parse and display one FIFO frame
 * @param frame_data Frame data buffer
 * @param timestamp_us Timestamp in microseconds
 * @return ESP_OK if frame is valid ACC+GYR frame, ESP_FAIL otherwise
 */
static esp_err_t parse_frame(const uint8_t *frame_data, int64_t timestamp_us)
{
    uint8_t header = frame_data[0];

    // Handle special headers
    if (header == FIFO_HEADER_CONFIG) {
        ESP_LOGD(TAG, "Config change frame (0x48)");
        g_config_frames++;
        return ESP_FAIL;
    }

    if (header == FIFO_HEADER_SKIP) {
        ESP_LOGW(TAG, "Skip frame (0x40) - data loss detected!");
        g_skip_frames++;
        return ESP_FAIL;
    }

    if (header != FIFO_HEADER_ACC_GYR) {
        ESP_LOGW(TAG, "Unknown header: 0x%02X", header);
        return ESP_FAIL;
    }

    // Parse gyroscope data FIRST (bytes 1-6)
    int16_t gyr_x = (int16_t)((frame_data[2] << 8) | frame_data[1]);
    int16_t gyr_y = (int16_t)((frame_data[4] << 8) | frame_data[3]);
    int16_t gyr_z = (int16_t)((frame_data[6] << 8) | frame_data[5]);

    // Parse accelerometer data SECOND (bytes 7-12)
    int16_t acc_x = (int16_t)((frame_data[8] << 8) | frame_data[7]);
    int16_t acc_y = (int16_t)((frame_data[10] << 8) | frame_data[9]);
    int16_t acc_z = (int16_t)((frame_data[12] << 8) | frame_data[11]);

    // Convert to physical values
    bmi270_raw_data_t gyr_raw = {gyr_x, gyr_y, gyr_z};
    bmi270_raw_data_t acc_raw = {acc_x, acc_y, acc_z};

    bmi270_gyro_t gyro;
    bmi270_accel_t accel;

    bmi270_convert_gyro_raw(&g_dev, &gyr_raw, &gyro);
    bmi270_convert_accel_raw(&g_dev, &acc_raw, &accel);

    // Teleplot output format with timestamp (only valid frames)
    printf(">gyr_x:%lld:%.2f\n", timestamp_us, gyro.x);
    printf(">gyr_y:%lld:%.2f\n", timestamp_us, gyro.y);
    printf(">gyr_z:%lld:%.2f\n", timestamp_us, gyro.z);
    printf(">acc_x:%lld:%.3f\n", timestamp_us, accel.x);
    printf(">acc_y:%lld:%.3f\n", timestamp_us, accel.y);
    printf(">acc_z:%lld:%.3f\n", timestamp_us, accel.z);

    g_valid_frames++;
    return ESP_OK;
}

/**
 * @brief Parse all frames in FIFO buffer
 */
static void parse_fifo_buffer(const uint8_t *buffer, uint16_t length)
{
    int num_frames = length / FIFO_FRAME_SIZE_HEADER;
    int valid_count = 0;

    ESP_LOGI(TAG, "Parsing %d frames (%u bytes)", num_frames, length);

    // Get current timestamp (for the most recent frame)
    int64_t base_time_us = esp_timer_get_time();

    // Calculate frame interval: 100Hz ODR = 10ms = 10000us per frame
    const int64_t frame_interval_us = 10000;

    for (int i = 0; i < num_frames; i++) {
        const uint8_t *frame = &buffer[i * FIFO_FRAME_SIZE_HEADER];
        g_total_frames++;

        // Calculate timestamp for this frame
        // FIFO is FIFO (first-in-first-out), so frame[0] is oldest
        // frame[num_frames-1] is newest (closest to base_time)
        int64_t frame_time_us = base_time_us - (num_frames - 1 - i) * frame_interval_us;

        if (parse_frame(frame, frame_time_us) == ESP_OK) {
            valid_count++;
        }
    }

    ESP_LOGI(TAG, "Valid frames: %d/%d", valid_count, num_frames);
}

void app_main(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " BMI270 Step 2: FIFO Multiple Frames Read");
    ESP_LOGI(TAG, "========================================");

    // Step 1: Initialize SPI bus
    ESP_LOGI(TAG, "Step 1: Initializing SPI bus...");
    bmi270_config_t config = {
        .gpio_mosi = BMI270_MOSI_PIN,
        .gpio_miso = BMI270_MISO_PIN,
        .gpio_sclk = BMI270_SCLK_PIN,
        .gpio_cs = BMI270_CS_PIN,
        .spi_clock_hz = BMI270_SPI_CLOCK_HZ,
        .spi_host = SPI2_HOST,
        .gpio_other_cs = PMW3901_CS_PIN
    };

    ret = bmi270_spi_init(&g_dev, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI");
        return;
    }
    ESP_LOGI(TAG, "SPI initialized successfully");

    // Step 2: Initialize BMI270
    ESP_LOGI(TAG, "Step 2: Initializing BMI270...");
    ret = bmi270_init(&g_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BMI270");
        return;
    }
    ESP_LOGI(TAG, "BMI270 initialized successfully");

    // Step 3: Set accelerometer to 100Hz, ±4g range
    ESP_LOGI(TAG, "Step 3: Configuring accelerometer (100Hz, ±4g)...");
    ret = bmi270_set_accel_config(&g_dev, BMI270_ACC_ODR_100HZ, BMI270_FILTER_PERFORMANCE);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set accelerometer config");
    }
    ESP_LOGI(TAG, "Accelerometer configured");

    // Step 4: Set gyroscope to 100Hz, ±1000°/s range
    ESP_LOGI(TAG, "Step 4: Configuring gyroscope (100Hz, ±1000°/s)...");
    ret = bmi270_set_gyro_config(&g_dev, BMI270_GYR_ODR_100HZ, BMI270_FILTER_PERFORMANCE);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set gyroscope config");
    }
    ESP_LOGI(TAG, "Gyroscope configured");

    // Wait for sensors to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));

    // Step 5: Configure FIFO (ACC+GYR, Header mode, Stream mode)
    ESP_LOGI(TAG, "Step 5: Configuring FIFO...");

    // FIFO_CONFIG_0: Stream mode (stop_on_full = 0, default 0x00)
    uint8_t fifo_config_0 = 0x00;
    ret = bmi270_write_register(&g_dev, BMI270_REG_FIFO_CONFIG_0, fifo_config_0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write FIFO_CONFIG_0");
        return;
    }

    // FIFO_CONFIG_1: Enable ACC+GYR, Header mode
    // bit 7: fifo_gyr_en = 1
    // bit 6: fifo_acc_en = 1
    // bit 4: fifo_header_en = 1
    uint8_t fifo_config_1 = (1 << 7) | (1 << 6) | (1 << 4);  // 0xD0
    ret = bmi270_write_register(&g_dev, BMI270_REG_FIFO_CONFIG_1, fifo_config_1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write FIFO_CONFIG_1");
        return;
    }

    ESP_LOGI(TAG, "FIFO configured: ACC+GYR enabled, Header mode, Stream mode");

    // Verify configuration
    uint8_t fifo_config_1_readback;
    bmi270_read_register(&g_dev, BMI270_REG_FIFO_CONFIG_1, &fifo_config_1_readback);
    ESP_LOGI(TAG, "FIFO_CONFIG_1 readback: 0x%02X (expected 0xD0)", fifo_config_1_readback);

    // Wait for some data to accumulate
    vTaskDelay(pdMS_TO_TICKS(200));

    // Step 6: Start FIFO multi-frame read loop
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " FIFO Multi-Frame Read Loop (Teleplot format)");
    ESP_LOGI(TAG, "========================================");

    static uint8_t fifo_buffer[FIFO_MAX_SIZE];  // Static to avoid stack overflow
    uint32_t loop_count = 0;

    while (1) {
        loop_count++;

        // Read FIFO length
        uint16_t fifo_length;
        ret = read_fifo_length(&fifo_length);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read FIFO length");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // Check if we have at least one frame (13 bytes)
        if (fifo_length >= FIFO_FRAME_SIZE_HEADER) {
            ESP_LOGI(TAG, "----------------------------------------");
            ESP_LOGI(TAG, "Loop #%lu, FIFO length: %u bytes", loop_count, fifo_length);

            // Read all FIFO data
            ret = read_fifo_data(fifo_buffer, fifo_length);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read FIFO data");
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }

            // Parse all frames in buffer
            parse_fifo_buffer(fifo_buffer, fifo_length);

            // Read FIFO length again to verify data was consumed
            uint16_t fifo_length_after;
            read_fifo_length(&fifo_length_after);
            ESP_LOGI(TAG, "FIFO length after read: %u bytes (consumed: %u bytes)",
                     fifo_length_after, fifo_length - fifo_length_after);

            // Statistics
            ESP_LOGI(TAG, "Statistics: Total=%lu Valid=%lu Skip=%lu Config=%lu",
                     g_total_frames, g_valid_frames, g_skip_frames, g_config_frames);
        }

        // Delay before next read (100ms = 10Hz polling)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
