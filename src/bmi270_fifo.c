/**
 * @file bmi270_fifo.c
 * @brief BMI270 FIFO API Implementation
 */

#include "bmi270_fifo.h"
#include "bmi270_spi.h"
#include "bmi270_defs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "BMI270_FIFO";

/**
 * @brief Configure BMI270 FIFO
 */
esp_err_t bmi270_configure_fifo(bmi270_dev_t *dev, const bmi270_fifo_config_t *config)
{
    if (dev == NULL || config == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;

    // Configure FIFO_CONFIG_0 (stop on full mode)
    uint8_t fifo_config_0 = config->stop_on_full ? BMI270_FIFO_STOP_ON_FULL : 0x00;
    ret = bmi270_write_register(dev, BMI270_REG_FIFO_CONFIG_0, fifo_config_0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write FIFO_CONFIG_0");
        return ret;
    }

    // Configure FIFO_CONFIG_1 (sensor enable, header mode)
    uint8_t fifo_config_1 = 0;
    if (config->acc_enable) fifo_config_1 |= BMI270_FIFO_ACC_EN;
    if (config->gyr_enable) fifo_config_1 |= BMI270_FIFO_GYR_EN;
    if (config->header_enable) fifo_config_1 |= BMI270_FIFO_HEADER_EN;

    ret = bmi270_write_register(dev, BMI270_REG_FIFO_CONFIG_1, fifo_config_1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write FIFO_CONFIG_1");
        return ret;
    }

    // Configure watermark (if non-zero)
    if (config->watermark > 0) {
        // BMI270 watermark register is in BYTE units (not 4-byte words!)
        // FIFO_WTM_0 (0x46): Lower 8 bits (fifo_water_mark_7_0)
        // FIFO_WTM_1 (0x47): Upper 5 bits (fifo_water_mark_12_8, bit 4-0)
        // Watermark level (bytes) = fifo_water_mark_7_0 + fifo_water_mark_12_8 × 256
        uint16_t watermark_value = config->watermark;

        // Maximum watermark is 2047 bytes (13-bit value, FIFO size is 2048 bytes)
        const uint16_t max_watermark = 2047;
        if (watermark_value > max_watermark) {
            ESP_LOGW(TAG, "Watermark %u bytes exceeds maximum %u bytes, clamping",
                     watermark_value, max_watermark);
            watermark_value = max_watermark;
        }

        // Write watermark LSB (bits 7-0)
        ret = bmi270_write_register(dev, BMI270_REG_FIFO_WTM_0, (uint8_t)(watermark_value & 0xFF));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write FIFO_WTM_0");
            return ret;
        }

        // Write watermark MSB (bits 12-8, only 5 bits used)
        ret = bmi270_write_register(dev, BMI270_REG_FIFO_WTM_1, (uint8_t)((watermark_value >> 8) & 0x1F));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write FIFO_WTM_1");
            return ret;
        }

        ESP_LOGI(TAG, "FIFO watermark set to %u bytes", watermark_value);
    }

    ESP_LOGI(TAG, "FIFO configured: acc=%d, gyr=%d, header=%d, stop_on_full=%d, watermark=%u",
             config->acc_enable, config->gyr_enable, config->header_enable,
             config->stop_on_full, config->watermark);

    return ESP_OK;
}

/**
 * @brief Get FIFO data length
 */
esp_err_t bmi270_get_fifo_length(bmi270_dev_t *dev, uint16_t *length)
{
    if (dev == NULL || length == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t length_data[2];
    esp_err_t ret = bmi270_read_burst(dev, BMI270_REG_FIFO_LENGTH_0, length_data, 2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read FIFO length");
        return ret;
    }

    // FIFO length is 11-bit (0-2047), stored in little-endian
    // The register value is in bytes (confirmed by testing)
    *length = (uint16_t)((length_data[1] << 8) | length_data[0]) & 0x07FF;

    return ESP_OK;
}

/**
 * @brief Read raw data from FIFO
 */
esp_err_t bmi270_read_fifo_data(bmi270_dev_t *dev, uint8_t *data, uint16_t length)
{
    if (dev == NULL || data == NULL || length == 0) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    if (length > BMI270_FIFO_SIZE) {
        ESP_LOGW(TAG, "Read length %u exceeds FIFO size %u", length, BMI270_FIFO_SIZE);
        return ESP_ERR_INVALID_SIZE;
    }

    // Read FIFO data using burst read
    esp_err_t ret = bmi270_read_burst(dev, BMI270_REG_FIFO_DATA, data, length);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read FIFO data");
        return ret;
    }

    return ESP_OK;
}

/**
 * @brief Parse a single FIFO frame from buffer
 */
esp_err_t bmi270_parse_fifo_frame(const uint8_t **buffer, uint16_t *length, bmi270_fifo_frame_t *frame)
{
    if (buffer == NULL || *buffer == NULL || length == NULL || frame == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    if (*length == 0) {
        return ESP_ERR_NOT_FOUND;  // No more data
    }

    const uint8_t *data = *buffer;
    uint16_t remaining = *length;

    // Read frame header
    uint8_t header = data[0];
    frame->type = (bmi270_fifo_frame_type_t)header;

    // Determine frame size and parse data
    uint16_t frame_size = 0;

    switch (header) {
        case BMI270_FIFO_HEAD_ACC:
            // Accelerometer frame: 1 header + 6 data bytes
            if (remaining < BMI270_FIFO_FRAME_ACC_SIZE) {
                ESP_LOGW(TAG, "Incomplete ACC frame (need %d, have %u)",
                         BMI270_FIFO_FRAME_ACC_SIZE, remaining);
                return ESP_ERR_INVALID_SIZE;
            }
            // Parse accelerometer data (little-endian)
            frame->acc.x = (int16_t)((data[2] << 8) | data[1]);
            frame->acc.y = (int16_t)((data[4] << 8) | data[3]);
            frame->acc.z = (int16_t)((data[6] << 8) | data[5]);
            frame_size = BMI270_FIFO_FRAME_ACC_SIZE;
            break;

        case BMI270_FIFO_HEAD_GYR:
            // Gyroscope frame: 1 header + 6 data bytes
            if (remaining < BMI270_FIFO_FRAME_GYR_SIZE) {
                ESP_LOGW(TAG, "Incomplete GYR frame (need %d, have %u)",
                         BMI270_FIFO_FRAME_GYR_SIZE, remaining);
                return ESP_ERR_INVALID_SIZE;
            }
            // Parse gyroscope data (little-endian)
            frame->gyr.x = (int16_t)((data[2] << 8) | data[1]);
            frame->gyr.y = (int16_t)((data[4] << 8) | data[3]);
            frame->gyr.z = (int16_t)((data[6] << 8) | data[5]);
            frame_size = BMI270_FIFO_FRAME_GYR_SIZE;
            break;

        case BMI270_FIFO_HEAD_ACC_GYR:
            // Accelerometer + Gyroscope frame: 1 header + 6 gyr + 6 acc bytes
            // NOTE: In BMI270 FIFO, gyroscope data comes FIRST, then accelerometer
            if (remaining < BMI270_FIFO_FRAME_ACC_GYR_SIZE) {
                ESP_LOGW(TAG, "Incomplete ACC+GYR frame (need %d, have %u)",
                         BMI270_FIFO_FRAME_ACC_GYR_SIZE, remaining);
                return ESP_ERR_INVALID_SIZE;
            }
            // Parse gyroscope data FIRST (bytes 1-6)
            frame->gyr.x = (int16_t)((data[2] << 8) | data[1]);
            frame->gyr.y = (int16_t)((data[4] << 8) | data[3]);
            frame->gyr.z = (int16_t)((data[6] << 8) | data[5]);
            // Parse accelerometer data SECOND (bytes 7-12)
            frame->acc.x = (int16_t)((data[8] << 8) | data[7]);
            frame->acc.y = (int16_t)((data[10] << 8) | data[9]);
            frame->acc.z = (int16_t)((data[12] << 8) | data[11]);
            frame_size = BMI270_FIFO_FRAME_ACC_GYR_SIZE;
            break;

        case BMI270_FIFO_HEAD_SKIP:
        case BMI270_FIFO_HEAD_SENSOR_TIME:
        case BMI270_FIFO_HEAD_CONFIG_CHANGE:
            // Skip these special frames (1 byte header only for now)
            // Note: Full implementation would parse sensor time (3 bytes) and config change
            ESP_LOGD(TAG, "Skipping special frame: 0x%02X", header);
            frame->type = BMI270_FIFO_FRAME_SKIP;
            frame_size = 1;
            break;

        default:
            // Unknown frame header
            ESP_LOGW(TAG, "Unknown FIFO frame header: 0x%02X", header);
            frame->type = BMI270_FIFO_FRAME_UNKNOWN;
            // Try to recover by skipping 1 byte
            frame_size = 1;
            return ESP_ERR_INVALID_RESPONSE;
    }

    // Advance buffer pointer and decrement length
    *buffer += frame_size;
    *length -= frame_size;

    return ESP_OK;
}

/**
 * @brief Flush FIFO buffer
 */
esp_err_t bmi270_flush_fifo(bmi270_dev_t *dev)
{
    if (dev == NULL) {
        ESP_LOGE(TAG, "Invalid device handle");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = bmi270_write_register(dev, BMI270_REG_CMD, BMI270_CMD_FIFO_FLUSH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to flush FIFO");
        return ret;
    }

    ESP_LOGD(TAG, "FIFO flushed");
    return ESP_OK;
}

/**
 * @brief Enable FIFO watermark interrupt
 */
esp_err_t bmi270_enable_fifo_watermark_interrupt(bmi270_dev_t *dev, bmi270_int_pin_t int_pin)
{
    if (dev == NULL) {
        ESP_LOGE(TAG, "Invalid device handle");
        return ESP_ERR_INVALID_ARG;
    }

    // FIFO watermark interrupt is mapped via INT_MAP_DATA register (0x58)
    // Note: INT1_MAP_FEAT/INT2_MAP_FEAT (0x56/0x57) are for advanced features only

    // Read current INT_MAP_DATA value
    uint8_t map_data;
    esp_err_t ret = bmi270_read_register(dev, BMI270_REG_INT_MAP_DATA, &map_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read INT_MAP_DATA register");
        return ret;
    }

    // Set FIFO watermark interrupt bit based on pin selection
    if (int_pin == BMI270_INT_PIN_1) {
        map_data |= (1 << 1);  // bit 1: FIFO watermark INT1
    } else {
        map_data |= (1 << 5);  // bit 5: FIFO watermark INT2
    }

    ret = bmi270_write_register(dev, BMI270_REG_INT_MAP_DATA, map_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write INT_MAP_DATA register");
        return ret;
    }

    ESP_LOGI(TAG, "FIFO watermark interrupt enabled on INT%d (INT_MAP_DATA: 0x%02X)",
             (int_pin == BMI270_INT_PIN_1) ? 1 : 2, map_data);
    return ESP_OK;
}
