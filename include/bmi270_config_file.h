/**
 * @file bmi270_config_file.h
 * @brief BMI270 Configuration File (8KB)
 *
 * This file contains the official Bosch Sensortec BMI270 configuration data.
 * This configuration must be uploaded to the BMI270 during initialization
 * to enable advanced features.
 *
 * Source: Bosch Sensortec BMI270 Configuration File
 * Size: 8192 bytes (0x2000)
 *
 * Usage:
 *   During BMI270 initialization, this data must be written to register
 *   INIT_DATA (0x5E) in 16-byte bursts.
 */

#ifndef BMI270_CONFIG_FILE_H
#define BMI270_CONFIG_FILE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Size of BMI270 configuration file in bytes */
#define BMI270_CONFIG_FILE_SIZE 8192

/** BMI270 configuration data array */
extern const uint8_t bmi270_config_file[BMI270_CONFIG_FILE_SIZE];

#ifdef __cplusplus
}
#endif

#endif // BMI270_CONFIG_FILE_H
