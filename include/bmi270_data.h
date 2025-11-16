/**
 * @file bmi270_data.h
 * @brief BMI270 Data Reading API
 *
 * This file provides functions for reading accelerometer and gyroscope
 * data from the BMI270 sensor.
 */

#ifndef BMI270_DATA_H
#define BMI270_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bmi270_spi.h"
#include <stdint.h>

/**
 * @brief Raw sensor data structure (16-bit integers)
 */
typedef struct {
    int16_t x;  ///< X-axis raw value
    int16_t y;  ///< Y-axis raw value
    int16_t z;  ///< Z-axis raw value
} bmi270_raw_data_t;

/**
 * @brief Accelerometer data in physical units (g)
 */
typedef struct {
    float x;    ///< X-axis acceleration [g]
    float y;    ///< Y-axis acceleration [g]
    float z;    ///< Z-axis acceleration [g]
} bmi270_accel_t;

/**
 * @brief Gyroscope data in physical units (°/s)
 */
typedef struct {
    float x;    ///< X-axis angular velocity [°/s]
    float y;    ///< Y-axis angular velocity [°/s]
    float z;    ///< Z-axis angular velocity [°/s]
} bmi270_gyro_t;

/**
 * @brief Accelerometer range settings
 */
typedef enum {
    BMI270_ACC_RANGE_2G  = 0x00,    ///< ±2g (16384 LSB/g)
    BMI270_ACC_RANGE_4G  = 0x01,    ///< ±4g (8192 LSB/g)
    BMI270_ACC_RANGE_8G  = 0x02,    ///< ±8g (4096 LSB/g)
    BMI270_ACC_RANGE_16G = 0x03     ///< ±16g (2048 LSB/g)
} bmi270_acc_range_t;

/**
 * @brief Gyroscope range settings
 */
typedef enum {
    BMI270_GYR_RANGE_125DPS  = 0x04,    ///< ±125°/s (262.4 LSB/°/s)
    BMI270_GYR_RANGE_250DPS  = 0x03,    ///< ±250°/s (131.2 LSB/°/s)
    BMI270_GYR_RANGE_500DPS  = 0x02,    ///< ±500°/s (65.6 LSB/°/s)
    BMI270_GYR_RANGE_1000DPS = 0x01,    ///< ±1000°/s (32.8 LSB/°/s)
    BMI270_GYR_RANGE_2000DPS = 0x00     ///< ±2000°/s (16.4 LSB/°/s)
} bmi270_gyr_range_t;

/**
 * @brief Output Data Rate (ODR) settings for accelerometer
 */
typedef enum {
    BMI270_ACC_ODR_0_78HZ  = 0x01,  ///< 0.78 Hz
    BMI270_ACC_ODR_1_5HZ   = 0x02,  ///< 1.5 Hz
    BMI270_ACC_ODR_3_1HZ   = 0x03,  ///< 3.1 Hz
    BMI270_ACC_ODR_6_25HZ  = 0x04,  ///< 6.25 Hz
    BMI270_ACC_ODR_12_5HZ  = 0x05,  ///< 12.5 Hz
    BMI270_ACC_ODR_25HZ    = 0x06,  ///< 25 Hz
    BMI270_ACC_ODR_50HZ    = 0x07,  ///< 50 Hz
    BMI270_ACC_ODR_100HZ   = 0x08,  ///< 100 Hz
    BMI270_ACC_ODR_200HZ   = 0x09,  ///< 200 Hz
    BMI270_ACC_ODR_400HZ   = 0x0A,  ///< 400 Hz
    BMI270_ACC_ODR_800HZ   = 0x0B,  ///< 800 Hz
    BMI270_ACC_ODR_1600HZ  = 0x0C   ///< 1600 Hz
} bmi270_acc_odr_t;

/**
 * @brief Output Data Rate (ODR) settings for gyroscope
 */
typedef enum {
    BMI270_GYR_ODR_25HZ   = 0x06,   ///< 25 Hz
    BMI270_GYR_ODR_50HZ   = 0x07,   ///< 50 Hz
    BMI270_GYR_ODR_100HZ  = 0x08,   ///< 100 Hz
    BMI270_GYR_ODR_200HZ  = 0x09,   ///< 200 Hz
    BMI270_GYR_ODR_400HZ  = 0x0A,   ///< 400 Hz
    BMI270_GYR_ODR_800HZ  = 0x0B,   ///< 800 Hz
    BMI270_GYR_ODR_1600HZ = 0x0C,   ///< 1600 Hz
    BMI270_GYR_ODR_3200HZ = 0x0D    ///< 3200 Hz
} bmi270_gyr_odr_t;

/**
 * @brief Filter performance mode
 */
typedef enum {
    BMI270_FILTER_POWER_OPT = 0,    ///< Power optimized
    BMI270_FILTER_PERFORMANCE = 1   ///< Performance mode
} bmi270_filter_perf_t;

/* ====== Data Reading Functions ====== */

/**
 * @brief Read raw accelerometer data (all 3 axes)
 *
 * This function reads all 3 axes in a single burst read transaction
 * to ensure data consistency (shadowing).
 *
 * @param[in]  dev   Pointer to BMI270 device structure
 * @param[out] data  Pointer to raw data structure
 * @return ESP_OK on success, error code otherwise
 *
 * @note Must call bmi270_init() before using this function
 */
esp_err_t bmi270_read_accel_raw(bmi270_dev_t *dev, bmi270_raw_data_t *data);

/**
 * @brief Read raw gyroscope data (all 3 axes)
 *
 * This function reads all 3 axes in a single burst read transaction
 * to ensure data consistency (shadowing).
 *
 * @param[in]  dev   Pointer to BMI270 device structure
 * @param[out] data  Pointer to raw data structure
 * @return ESP_OK on success, error code otherwise
 *
 * @note Must call bmi270_init() before using this function
 */
esp_err_t bmi270_read_gyro_raw(bmi270_dev_t *dev, bmi270_raw_data_t *data);

/**
 * @brief Read accelerometer data in physical units (g)
 *
 * This function reads raw data and converts it to physical units
 * based on the current range setting.
 *
 * @param[in]  dev   Pointer to BMI270 device structure
 * @param[out] data  Pointer to accelerometer data structure (units: g)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bmi270_read_accel(bmi270_dev_t *dev, bmi270_accel_t *data);

/**
 * @brief Read gyroscope data in physical units (°/s)
 *
 * This function reads raw data and converts it to physical units
 * based on the current range setting.
 *
 * @param[in]  dev   Pointer to BMI270 device structure
 * @param[out] data  Pointer to gyroscope data structure (units: °/s)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bmi270_read_gyro(bmi270_dev_t *dev, bmi270_gyro_t *data);

/**
 * @brief Read temperature sensor data in °C
 *
 * This function reads raw temperature data and converts it to physical units.
 * Temperature sensor must be enabled (TEMP_EN bit in PWR_CTRL register).
 *
 * @param[in]  dev         Pointer to BMI270 device structure
 * @param[out] temperature Pointer to store temperature value (units: °C)
 * @return ESP_OK on success, error code otherwise
 *
 * @note Temperature resolution: approximately 1/512 °C per LSB
 * @note Typical range: -40°C to +85°C
 */
esp_err_t bmi270_read_temperature(bmi270_dev_t *dev, float *temperature);

/* ====== Configuration Functions ====== */

/**
 * @brief Configure accelerometer range
 *
 * @param[in] dev    Pointer to BMI270 device structure
 * @param[in] range  Accelerometer range setting
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bmi270_set_accel_range(bmi270_dev_t *dev, bmi270_acc_range_t range);

/**
 * @brief Configure gyroscope range
 *
 * @param[in] dev    Pointer to BMI270 device structure
 * @param[in] range  Gyroscope range setting
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bmi270_set_gyro_range(bmi270_dev_t *dev, bmi270_gyr_range_t range);

/**
 * @brief Configure accelerometer ODR and filter performance
 *
 * @param[in] dev         Pointer to BMI270 device structure
 * @param[in] odr         Output data rate
 * @param[in] filter_perf Filter performance mode
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bmi270_set_accel_config(bmi270_dev_t *dev,
                                   bmi270_acc_odr_t odr,
                                   bmi270_filter_perf_t filter_perf);

/**
 * @brief Configure gyroscope ODR and filter performance
 *
 * @param[in] dev         Pointer to BMI270 device structure
 * @param[in] odr         Output data rate
 * @param[in] filter_perf Filter performance mode
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bmi270_set_gyro_config(bmi270_dev_t *dev,
                                  bmi270_gyr_odr_t odr,
                                  bmi270_filter_perf_t filter_perf);

/**
 * @brief Get current accelerometer range setting
 *
 * @param[in]  dev    Pointer to BMI270 device structure
 * @param[out] range  Pointer to store current range
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bmi270_get_accel_range(bmi270_dev_t *dev, bmi270_acc_range_t *range);

/**
 * @brief Get current gyroscope range setting
 *
 * @param[in]  dev    Pointer to BMI270 device structure
 * @param[out] range  Pointer to store current range
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t bmi270_get_gyro_range(bmi270_dev_t *dev, bmi270_gyr_range_t *range);

#ifdef __cplusplus
}
#endif

#endif // BMI270_DATA_H
