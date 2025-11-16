/**
 * @file bmi270_defs.h
 * @brief BMI270 Register definitions and constants
 *
 * This file contains all register addresses, bit masks, and constants
 * for the BMI270 6-axis IMU sensor.
 */

#ifndef BMI270_DEFS_H
#define BMI270_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ====== BMI270 Register Addresses ====== */

/* Chip Identification */
#define BMI270_REG_CHIP_ID              0x00    // Chip ID register (should read 0x24)

/* Error and Status */
#define BMI270_REG_ERR_REG              0x02    // Error register
#define BMI270_REG_STATUS               0x03    // Status register

/* Sensor Data Registers */
#define BMI270_REG_ACC_X_LSB            0x0C    // Accelerometer X-axis LSB
#define BMI270_REG_ACC_X_MSB            0x0D    // Accelerometer X-axis MSB
#define BMI270_REG_ACC_Y_LSB            0x0E    // Accelerometer Y-axis LSB
#define BMI270_REG_ACC_Y_MSB            0x0F    // Accelerometer Y-axis MSB
#define BMI270_REG_ACC_Z_LSB            0x10    // Accelerometer Z-axis LSB
#define BMI270_REG_ACC_Z_MSB            0x11    // Accelerometer Z-axis MSB

#define BMI270_REG_GYR_X_LSB            0x12    // Gyroscope X-axis LSB
#define BMI270_REG_GYR_X_MSB            0x13    // Gyroscope X-axis MSB
#define BMI270_REG_GYR_Y_LSB            0x14    // Gyroscope Y-axis LSB
#define BMI270_REG_GYR_Y_MSB            0x15    // Gyroscope Y-axis MSB
#define BMI270_REG_GYR_Z_LSB            0x16    // Gyroscope Z-axis LSB
#define BMI270_REG_GYR_Z_MSB            0x17    // Gyroscope Z-axis MSB

/* Internal Status */
#define BMI270_REG_INTERNAL_STATUS      0x21    // Internal status register

/* FIFO */
#define BMI270_REG_FIFO_LENGTH_0        0x24    // FIFO length LSB
#define BMI270_REG_FIFO_LENGTH_1        0x25    // FIFO length MSB
#define BMI270_REG_FIFO_DATA            0x26    // FIFO data read

/* Configuration Registers */
#define BMI270_REG_ACC_CONF             0x40    // Accelerometer configuration
#define BMI270_REG_GYR_CONF             0x42    // Gyroscope configuration

/* Initialization Registers */
#define BMI270_REG_INIT_CTRL            0x59    // Initialization control
#define BMI270_REG_INIT_ADDR_0          0x5B    // Init address LSB
#define BMI270_REG_INIT_ADDR_1          0x5C    // Init address MSB
#define BMI270_REG_INIT_DATA            0x5E    // Init data register

/* Power Registers */
#define BMI270_REG_PWR_CONF             0x7C    // Power configuration
#define BMI270_REG_PWR_CTRL             0x7D    // Power control

/* Command Register */
#define BMI270_REG_CMD                  0x7E    // Command register


/* ====== BMI270 Constants ====== */

/* Chip ID */
#define BMI270_CHIP_ID                  0x24    // Expected CHIP_ID value

/* SPI Communication */
#define BMI270_SPI_READ_BIT             0x80    // SPI read bit (bit 7 = 1)
#define BMI270_SPI_WRITE_BIT            0x00    // SPI write bit (bit 7 = 0)

/* Internal Status - Message Field */
#define BMI270_INTERNAL_STATUS_MSG_MASK         0x0F    // Message field mask
#define BMI270_INTERNAL_STATUS_MSG_NOT_INIT     0x00    // Not initialized
#define BMI270_INTERNAL_STATUS_MSG_INIT_OK      0x01    // Initialization OK
#define BMI270_INTERNAL_STATUS_MSG_INIT_ERR     0x02    // Initialization error

/* Initialization Control Commands */
#define BMI270_INIT_CTRL_PREPARE        0x00    // Prepare for config file upload
#define BMI270_INIT_CTRL_COMPLETE       0x01    // Config file upload complete

/* Power Configuration */
#define BMI270_PWR_CONF_ADV_PWR_SAVE_EN 0x00    // Disable advanced power save (for init)
#define BMI270_PWR_CONF_NORMAL          0x02    // Normal power mode

/* Power Control Bits */
#define BMI270_PWR_CTRL_AUX_EN          (1 << 0)    // Enable auxiliary sensor
#define BMI270_PWR_CTRL_GYR_EN          (1 << 1)    // Enable gyroscope
#define BMI270_PWR_CTRL_ACC_EN          (1 << 2)    // Enable accelerometer
#define BMI270_PWR_CTRL_TEMP_EN         (1 << 3)    // Enable temperature sensor

/* Commands */
#define BMI270_CMD_SOFT_RESET           0xB6    // Soft reset command

/* Timing Constants (microseconds) */
#define BMI270_DELAY_POWER_ON_US        450     // Power-on delay
#define BMI270_DELAY_SOFT_RESET_US      2000    // Soft reset delay
#define BMI270_DELAY_WRITE_NORMAL_US    2       // Write delay in normal mode
#define BMI270_DELAY_WRITE_SUSPEND_US   450     // Write delay in suspend mode (minimum)
#define BMI270_DELAY_ACCESS_LOWPOWER_US 1000    // Register access delay in low-power mode (optimized: 100% reliable, good speed/safety balance)

/* Timeouts (milliseconds) */
#define BMI270_TIMEOUT_INIT_MS          150     // Initialization timeout (increased for safety)

/* Configuration File */
#define BMI270_CONFIG_FILE_SIZE         8192    // Size of config file in bytes
#define BMI270_CONFIG_BURST_SIZE        256     // Burst write size for config upload (bytes, proven reliable)


#ifdef __cplusplus
}
#endif

#endif // BMI270_DEFS_H
