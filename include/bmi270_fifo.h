/**
 * @file bmi270_fifo.h
 * @brief BMI270 FIFO API
 *
 * This file provides APIs for BMI270 FIFO functionality including:
 * - FIFO configuration (enable sensors, watermark, header mode)
 * - FIFO data reading and frame parsing
 * - FIFO flush and status checking
 */

#ifndef BMI270_FIFO_H
#define BMI270_FIFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bmi270_types.h"
#include "bmi270_data.h"
#include "bmi270_interrupt.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief FIFO configuration structure
 */
typedef struct {
    bool acc_enable;        ///< Enable accelerometer data in FIFO
    bool gyr_enable;        ///< Enable gyroscope data in FIFO
    bool header_enable;     ///< Enable frame headers in FIFO
    bool stop_on_full;      ///< FIFO stops on full (true) or overwrites (false)
    uint16_t watermark;     ///< FIFO watermark threshold (bytes, 0-1023)
} bmi270_fifo_config_t;

/**
 * @brief FIFO frame type enumeration
 */
typedef enum {
    BMI270_FIFO_FRAME_SKIP = 0x40,          ///< Skip frame
    BMI270_FIFO_FRAME_SENSOR_TIME = 0x44,   ///< Sensor time frame
    BMI270_FIFO_FRAME_CONFIG_CHANGE = 0x48, ///< Configuration change
    BMI270_FIFO_FRAME_ACC = 0x84,           ///< Accelerometer frame
    BMI270_FIFO_FRAME_GYR = 0x88,           ///< Gyroscope frame
    BMI270_FIFO_FRAME_ACC_GYR = 0x8C,       ///< Accelerometer + Gyroscope frame
    BMI270_FIFO_FRAME_UNKNOWN = 0xFF        ///< Unknown/invalid frame
} bmi270_fifo_frame_type_t;

/**
 * @brief FIFO frame structure (parsed data)
 */
typedef struct {
    bmi270_fifo_frame_type_t type;  ///< Frame type
    bmi270_raw_data_t acc;          ///< Accelerometer data (valid if type includes ACC)
    bmi270_raw_data_t gyr;          ///< Gyroscope data (valid if type includes GYR)
} bmi270_fifo_frame_t;

/**
 * @brief Configure BMI270 FIFO
 *
 * @param[in] dev Device handle
 * @param[in] config FIFO configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bmi270_configure_fifo(bmi270_dev_t *dev, const bmi270_fifo_config_t *config);

/**
 * @brief Get FIFO data length
 *
 * @param[in] dev Device handle
 * @param[out] length Number of bytes available in FIFO
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bmi270_get_fifo_length(bmi270_dev_t *dev, uint16_t *length);

/**
 * @brief Read raw data from FIFO
 *
 * This function reads raw bytes from the FIFO buffer.
 * Use bmi270_parse_fifo_frame() to parse the data into frames.
 *
 * @param[in] dev Device handle
 * @param[out] data Buffer to store FIFO data
 * @param[in] length Number of bytes to read
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bmi270_read_fifo_data(bmi270_dev_t *dev, uint8_t *data, uint16_t length);

/**
 * @brief Parse a single FIFO frame from buffer
 *
 * This function parses one frame from the FIFO data buffer.
 * It advances the buffer pointer and decrements the length accordingly.
 *
 * @param[in,out] buffer Pointer to FIFO data buffer (will be advanced)
 * @param[in,out] length Remaining buffer length (will be decremented)
 * @param[out] frame Parsed frame data
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if no more frames, error code otherwise
 */
esp_err_t bmi270_parse_fifo_frame(const uint8_t **buffer, uint16_t *length, bmi270_fifo_frame_t *frame);

/**
 * @brief Flush FIFO buffer
 *
 * This command clears all data in the FIFO buffer.
 *
 * @param[in] dev Device handle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bmi270_flush_fifo(bmi270_dev_t *dev);

/**
 * @brief Enable FIFO watermark interrupt
 *
 * When enabled, the BMI270 generates an interrupt when the FIFO
 * reaches the configured watermark threshold.
 *
 * @param[in] dev Device handle
 * @param[in] int_pin Interrupt pin (INT1 or INT2)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bmi270_enable_fifo_watermark_interrupt(bmi270_dev_t *dev, bmi270_int_pin_t int_pin);

#ifdef __cplusplus
}
#endif

#endif // BMI270_FIFO_H
