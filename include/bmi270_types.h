/**
 * @file bmi270_types.h
 * @brief BMI270 Type definitions and structures
 *
 * This file contains all type definitions, structures, and enumerations
 * for the BMI270 driver.
 */

#ifndef BMI270_TYPES_H
#define BMI270_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/spi_master.h"

/**
 * @brief BMI270 device structure
 *
 * This structure holds all necessary information for communicating
 * with the BMI270 sensor via SPI.
 */
typedef struct {
    spi_device_handle_t spi_handle;     ///< ESP-IDF SPI device handle
    uint8_t gpio_mosi;                   ///< MOSI GPIO pin number
    uint8_t gpio_miso;                   ///< MISO GPIO pin number
    uint8_t gpio_sclk;                   ///< SCLK GPIO pin number
    uint8_t gpio_cs;                     ///< CS GPIO pin number
    uint32_t spi_clock_hz;               ///< SPI clock frequency in Hz
    bool initialized;                    ///< SPI initialized (bus setup complete)
    bool init_complete;                  ///< BMI270 initialization complete (normal mode)
} bmi270_dev_t;

/**
 * @brief BMI270 configuration structure for initialization
 */
typedef struct {
    uint8_t gpio_mosi;                   ///< MOSI GPIO pin (e.g., GPIO14 for StampFly)
    uint8_t gpio_miso;                   ///< MISO GPIO pin (e.g., GPIO43 for StampFly)
    uint8_t gpio_sclk;                   ///< SCLK GPIO pin (e.g., GPIO44 for StampFly)
    uint8_t gpio_cs;                     ///< CS GPIO pin (e.g., GPIO46 for StampFly)
    uint32_t spi_clock_hz;               ///< SPI clock frequency (max 10MHz for BMI270)
    spi_host_device_t spi_host;          ///< SPI host (SPI2_HOST or SPI3_HOST)
} bmi270_config_t;

/**
 * @brief BMI270 sensor data structure
 *
 * Contains raw 16-bit sensor data from accelerometer and gyroscope.
 */
typedef struct {
    int16_t acc_x;      ///< Accelerometer X-axis [LSB]
    int16_t acc_y;      ///< Accelerometer Y-axis [LSB]
    int16_t acc_z;      ///< Accelerometer Z-axis [LSB]
    int16_t gyr_x;      ///< Gyroscope X-axis [LSB]
    int16_t gyr_y;      ///< Gyroscope Y-axis [LSB]
    int16_t gyr_z;      ///< Gyroscope Z-axis [LSB]
} bmi270_sensor_data_t;

#ifdef __cplusplus
}
#endif

#endif // BMI270_TYPES_H
