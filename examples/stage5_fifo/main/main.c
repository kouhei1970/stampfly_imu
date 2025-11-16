/**
 * @file main.c
 * @brief BMI270 Stage 5: FIFO High-Speed Data Read Example
 *
 * This example demonstrates:
 * - BMI270 initialization with FIFO configuration
 * - FIFO watermark interrupt setup
 * - Batch reading sensor data from FIFO
 * - Parsing FIFO frames (accelerometer + gyroscope)
 * - Efficient data acquisition with reduced SPI transactions
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "bmi270_spi.h"
#include "bmi270_init.h"
#include "bmi270_data.h"
#include "bmi270_interrupt.h"
#include "bmi270_fifo.h"

static const char *TAG = "BMI270_STAGE5";

// M5StampFly BMI270 pin configuration
#define BMI270_MOSI_PIN     14
#define BMI270_MISO_PIN     43
#define BMI270_SCLK_PIN     44
#define BMI270_CS_PIN       46
#define BMI270_SPI_CLOCK_HZ 10000000  // 10 MHz
#define PMW3901_CS_PIN      12        // Other device on shared SPI bus

// Output options
#define OUTPUT_RAW_VALUES   1  // Set to 1 to output raw sensor values (LSB)

// BMI270 INT1 pin connected to ESP32 GPIO
#define BMI270_INT1_GPIO    GPIO_NUM_11  // INT1 pin from BMI270 (per M5StampFly hardware)

// FIFO configuration
#define FIFO_WATERMARK      512  // Watermark threshold (bytes): 512 / 13 ≈ 39 frames

// Global device handle and interrupt queue
static bmi270_dev_t g_dev = {0};
static QueueHandle_t gpio_evt_queue = NULL;

/**
 * @brief GPIO interrupt handler (IRAM_ATTR for fast execution)
 *
 * This ISR is called when BMI270's INT1 pin triggers (FIFO watermark reached).
 * It simply sends a notification to the main task via queue.
 */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

/**
 * @brief Configure ESP32 GPIO for BMI270 INT1 interrupt
 */
static esp_err_t setup_gpio_interrupt(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,      // Trigger on rising edge (INT1 is active high)
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BMI270_INT1_GPIO),
        .pull_down_en = GPIO_PULLDOWN_ENABLE,  // Pull-down to avoid floating
        .pull_up_en = GPIO_PULLUP_DISABLE
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO");
        return ret;
    }

    // Create queue for GPIO events
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    if (gpio_evt_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create GPIO event queue");
        return ESP_ERR_NO_MEM;
    }

    // Install GPIO ISR service
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {  // ESP_ERR_INVALID_STATE = already installed
        ESP_LOGE(TAG, "Failed to install GPIO ISR service");
        return ret;
    }

    // Attach interrupt handler to GPIO
    ret = gpio_isr_handler_add(BMI270_INT1_GPIO, gpio_isr_handler, (void*) BMI270_INT1_GPIO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add GPIO ISR handler");
        return ret;
    }

    ESP_LOGI(TAG, "GPIO interrupt configured on GPIO %d", BMI270_INT1_GPIO);
    return ESP_OK;
}

void app_main(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " BMI270 Stage 5: FIFO Data Reading");
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

    ret = bmi270_spi_init(&g_dev, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ SPI initialization failed");
        return;
    }
    ESP_LOGI(TAG, "✓ SPI initialized successfully");
    ESP_LOGI(TAG, "");

    // Step 2: Activate SPI mode
    ESP_LOGI(TAG, "Step 2: Activating SPI mode...");
    uint8_t dummy;
    bmi270_read_register(&g_dev, BMI270_REG_CHIP_ID, &dummy);  // First dummy read
    vTaskDelay(pdMS_TO_TICKS(5));  // Wait 5ms
    bmi270_read_register(&g_dev, BMI270_REG_CHIP_ID, &dummy);  // Second dummy read
    ESP_LOGI(TAG, "SPI mode activated");
    ESP_LOGI(TAG, "");

    // Step 3: Initialize BMI270
    ESP_LOGI(TAG, "Step 3: Initializing BMI270...");
    ret = bmi270_init(&g_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ BMI270 initialization failed");
        return;
    }
    ESP_LOGI(TAG, "✓ BMI270 initialized successfully");
    ESP_LOGI(TAG, "");

    // Step 4: Configure sensor settings
    ESP_LOGI(TAG, "Step 4: Configuring sensors...");

    // Set accelerometer to ±4g range
    ret = bmi270_set_accel_range(&g_dev, BMI270_ACC_RANGE_4G);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set accelerometer range");
    } else {
        ESP_LOGI(TAG, "Accelerometer range set to ±4g");
    }

    // Set gyroscope to ±1000 °/s range
    ret = bmi270_set_gyro_range(&g_dev, BMI270_GYR_RANGE_1000DPS);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set gyroscope range");
    } else {
        ESP_LOGI(TAG, "Gyroscope range set to ±1000 °/s");
    }

    // Set accelerometer to 100Hz, Performance mode
    ret = bmi270_set_accel_config(&g_dev, BMI270_ACC_ODR_100HZ, BMI270_FILTER_PERFORMANCE);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set accelerometer config");
    } else {
        ESP_LOGI(TAG, "Accelerometer configured: 100Hz, Performance mode");
    }

    // Set gyroscope to 100Hz (same as accel) to generate combined frames in FIFO
    // This reduces FIFO data rate from 2100 bytes/s to 1300 bytes/s
    ret = bmi270_set_gyro_config(&g_dev, BMI270_GYR_ODR_100HZ, BMI270_FILTER_PERFORMANCE);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set gyroscope config");
    } else {
        ESP_LOGI(TAG, "Gyroscope configured: 100Hz, Performance mode (same as accel)");
    }

    ESP_LOGI(TAG, "✓ Sensor configuration complete");
    ESP_LOGI(TAG, "");

    // Step 5: Configure INT1 pin
    ESP_LOGI(TAG, "Step 5: Configuring BMI270 INT1 pin...");
    bmi270_int_pin_config_t int_config = {
        .output_enable = true,
        .active_high = true,     // Active high (rises on interrupt)
        .open_drain = false      // Push-pull output
    };

    ret = bmi270_configure_int_pin(&g_dev, BMI270_INT_PIN_1, &int_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to configure INT1 pin");
        return;
    }
    ESP_LOGI(TAG, "✓ INT1 pin configured (Active High, Push-Pull)");
    ESP_LOGI(TAG, "");

    // Step 6: Setup ESP32 GPIO interrupt
    ESP_LOGI(TAG, "Step 6: Setting up ESP32 GPIO interrupt...");
    ret = setup_gpio_interrupt();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to setup GPIO interrupt");
        return;
    }
    ESP_LOGI(TAG, "✓ GPIO interrupt configured on GPIO %d", BMI270_INT1_GPIO);
    ESP_LOGI(TAG, "");

    // Step 7: Configure FIFO
    ESP_LOGI(TAG, "Step 7: Configuring FIFO...");
    bmi270_fifo_config_t fifo_config = {
        .acc_enable = true,
        .gyr_enable = true,
        .header_enable = true,
        .stop_on_full = true,
        .watermark = FIFO_WATERMARK
    };

    ret = bmi270_configure_fifo(&g_dev, &fifo_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to configure FIFO");
        return;
    }
    ESP_LOGI(TAG, "✓ FIFO configured (watermark: %u bytes)", FIFO_WATERMARK);
    ESP_LOGI(TAG, "");

    // Step 8: Enable FIFO watermark interrupt
    ESP_LOGI(TAG, "Step 8: Enabling FIFO watermark interrupt...");
    ret = bmi270_set_int_latch_mode(&g_dev, false);  // Pulse mode (non-latched)
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to set latch mode");
        return;
    }

    ret = bmi270_enable_fifo_watermark_interrupt(&g_dev, BMI270_INT_PIN_1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ Failed to enable FIFO watermark interrupt");
        return;
    }
    ESP_LOGI(TAG, "✓ FIFO watermark interrupt enabled on INT1");
    ESP_LOGI(TAG, "");

    // Step 9: Flush FIFO and verify configuration
    ESP_LOGI(TAG, "Step 9: Flushing FIFO and verifying configuration...");

    // Flush FIFO to start fresh
    ret = bmi270_flush_fifo(&g_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to flush FIFO");
        return;
    }
    ESP_LOGI(TAG, "FIFO flushed");

    // Read FIFO configuration and interrupt mapping registers
    uint8_t fifo_config_0, fifo_config_1, int_map_data, int1_io_ctrl;
    uint8_t fifo_wtm_0, fifo_wtm_1;
    bmi270_read_register(&g_dev, 0x48, &fifo_config_0);
    bmi270_read_register(&g_dev, 0x49, &fifo_config_1);
    bmi270_read_register(&g_dev, 0x58, &int_map_data);  // Correct register for FIFO interrupts
    bmi270_read_register(&g_dev, 0x53, &int1_io_ctrl);
    bmi270_read_register(&g_dev, 0x46, &fifo_wtm_0);
    bmi270_read_register(&g_dev, 0x47, &fifo_wtm_1);

    // Watermark register is in BYTE units (not 4-byte words)
    uint16_t wtm_bytes = (uint16_t)((fifo_wtm_1 << 8) | fifo_wtm_0);
    ESP_LOGI(TAG, "FIFO_CONFIG_0: 0x%02X, FIFO_CONFIG_1: 0x%02X", fifo_config_0, fifo_config_1);
    ESP_LOGI(TAG, "INT_MAP_DATA: 0x%02X (should be 0x02), INT1_IO_CTRL: 0x%02X", int_map_data, int1_io_ctrl);
    ESP_LOGI(TAG, "FIFO_WTM: 0x%02X%02X = %u bytes (watermark register is in byte units)",
             fifo_wtm_1, fifo_wtm_0, wtm_bytes);

    // Verify FIFO is empty after flush
    uint16_t fifo_length;
    ret = bmi270_get_fifo_length(&g_dev, &fifo_length);
    ESP_LOGI(TAG, "FIFO length after flush: %u bytes (should be 0)", fifo_length);

    // Wait for data to accumulate
    vTaskDelay(pdMS_TO_TICKS(500));
    ret = bmi270_get_fifo_length(&g_dev, &fifo_length);
    ESP_LOGI(TAG, "FIFO length after 500ms: %u bytes", fifo_length);
    ESP_LOGI(TAG, "Expected watermark trigger at: %u bytes", FIFO_WATERMARK);
    ESP_LOGI(TAG, "");

    // Step 10: Start FIFO data reading
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " FIFO Data Stream (Teleplot format)");
    ESP_LOGI(TAG, "========================================");

    uint32_t gpio_num;
    uint32_t batch_count = 0;
    static uint8_t fifo_buffer[BMI270_FIFO_SIZE];  // 2048 bytes buffer (BSS section, not stack)

    while (1) {
        // Wait for FIFO watermark interrupt (blocking, with timeout for debugging)
        if (xQueueReceive(gpio_evt_queue, &gpio_num, pdMS_TO_TICKS(2000))) {
            batch_count++;

            // Read FIFO length
            uint16_t fifo_length;
            ret = bmi270_get_fifo_length(&g_dev, &fifo_length);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read FIFO length");
                continue;
            }

            ESP_LOGI(TAG, "FIFO length: %u bytes", fifo_length);

            if (fifo_length == 0) {
                ESP_LOGW(TAG, "FIFO is empty");
                continue;
            }

            // Read FIFO data
            ret = bmi270_read_fifo_data(&g_dev, fifo_buffer, fifo_length);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read FIFO data");
                continue;
            }

            // Parse FIFO frames
            const uint8_t *buffer_ptr = fifo_buffer;
            uint16_t remaining = fifo_length;
            uint32_t frame_count = 0;
            uint32_t acc_frames = 0, gyr_frames = 0, acc_gyr_frames = 0, other_frames = 0;

            while (remaining > 0) {
                bmi270_fifo_frame_t frame;
                ret = bmi270_parse_fifo_frame(&buffer_ptr, &remaining, &frame);

                if (ret != ESP_OK) {
                    if (ret == ESP_ERR_NOT_FOUND) {
                        break;  // No more frames
                    }
                    ESP_LOGW(TAG, "Frame parse error");
                    continue;
                }

                // Count frame types for debugging
                if (frame.type == BMI270_FIFO_FRAME_ACC) {
                    acc_frames++;
                } else if (frame.type == BMI270_FIFO_FRAME_GYR) {
                    gyr_frames++;
                } else if (frame.type == BMI270_FIFO_FRAME_ACC_GYR) {
                    acc_gyr_frames++;
                } else {
                    other_frames++;
                }

                // Process only accelerometer + gyroscope frames
                if (frame.type == BMI270_FIFO_FRAME_ACC_GYR) {
                    frame_count++;

                    // Convert raw data to physical values
                    bmi270_accel_t accel;
                    bmi270_gyro_t gyro;

                    bmi270_convert_accel_raw(&g_dev, &frame.acc, &accel);
                    bmi270_convert_gyro_raw(&g_dev, &frame.gyr, &gyro);

                    // Teleplot output format
#if OUTPUT_RAW_VALUES
                    // Accelerometer raw (LSB)
                    printf(">acc_raw_x:%d\n", frame.acc.x);
                    printf(">acc_raw_y:%d\n", frame.acc.y);
                    printf(">acc_raw_z:%d\n", frame.acc.z);
#endif

                    // Accelerometer physical (g)
                    printf(">acc_x:%.4f\n", accel.x);
                    printf(">acc_y:%.4f\n", accel.y);
                    printf(">acc_z:%.4f\n", accel.z);

#if OUTPUT_RAW_VALUES
                    // Gyroscope raw (LSB)
                    printf(">gyr_raw_x:%d\n", frame.gyr.x);
                    printf(">gyr_raw_y:%d\n", frame.gyr.y);
                    printf(">gyr_raw_z:%d\n", frame.gyr.z);
#endif

                    // Gyroscope physical (°/s)
                    printf(">gyr_x:%.3f\n", gyro.x);
                    printf(">gyr_y:%.3f\n", gyro.y);
                    printf(">gyr_z:%.3f\n", gyro.z);
                }
            }

            // Read temperature (once per batch)
            float temperature;
            ret = bmi270_read_temperature(&g_dev, &temperature);
            if (ret == ESP_OK) {
#if OUTPUT_RAW_VALUES
                int16_t temp_raw = (int16_t)((temperature - 23.0f) * 512.0f);
                printf(">temp_raw:%d\n", temp_raw);
#endif
                printf(">temp:%.2f\n", temperature);
            }

            // Output frame count
            printf(">fifo_count:%lu\n", frame_count);

            // Log batch processing with frame type breakdown
            ESP_LOGI(TAG, "FIFO watermark reached: Total=%lu (ACC=%lu, GYR=%lu, ACC+GYR=%lu, Other=%lu)",
                     acc_frames + gyr_frames + acc_gyr_frames + other_frames,
                     acc_frames, gyr_frames, acc_gyr_frames, other_frames);

            // NOTE: Do NOT flush FIFO here - FIFO_DATA register read should auto-advance pointer
            // Flushing here causes data loss (only 10 frames instead of expected 39)
            // If FIFO accumulates, the problem is with FIFO_DATA register read implementation
        } else {
            // Timeout - no interrupt received, check FIFO status
            uint16_t fifo_length;
            ret = bmi270_get_fifo_length(&g_dev, &fifo_length);
            if (ret == ESP_OK) {
                ESP_LOGW(TAG, "Timeout waiting for interrupt. FIFO length: %u bytes", fifo_length);

                // If FIFO has data, read it anyway (interrupt issue)
                if (fifo_length >= FIFO_WATERMARK) {
                    ESP_LOGW(TAG, "FIFO has data but no interrupt! Reading manually...");
                    // Continue to process (similar to interrupt case)
                    // This indicates an interrupt configuration problem
                }
            } else {
                ESP_LOGE(TAG, "Failed to read FIFO length");
            }
        }
    }
}
