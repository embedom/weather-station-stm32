/**
 ******************************************************************************
 * @file        : bme280_enumerations.hpp
 * @author      : embedom
 * @date        : 2026-04-16
 * @brief       : BME280 sensor registers definitions.
 ******************************************************************************
 */

#pragma once

/********************************* INCLUDES **********************************/

#include <cstdint>

namespace BME280
{

/******************************** CONSTEXPR **********************************/

constexpr uint8_t SENSOR_REG_ID = 0xD0U;
constexpr uint8_t SENSOR_REG_RESET = 0xE0U;
constexpr uint8_t SENSOR_REG_CTRL_HUM = 0xF2U;
constexpr uint8_t SENSOR_REG_STATUS = 0xF3U;
constexpr uint8_t SENSOR_REG_CTRL_MEAS = 0xF4U;
constexpr uint8_t SENSOR_REG_CONFIG = 0xF5U;
constexpr uint8_t SENSOR_REG_PRESS_MSB = 0xF7U;
constexpr uint8_t SENSOR_REG_PRESS_LSB = 0xF8U;
constexpr uint8_t SENSOR_REG_PRESS_XLSB = 0xF9U;
constexpr uint8_t SENSOR_REG_TEMP_MSB = 0xFAU;
constexpr uint8_t SENSOR_REG_TEMP_LSB = 0xFBU;
constexpr uint8_t SENSOR_REG_TEMP_XLSB = 0xFCU;
constexpr uint8_t SENSOR_REG_HUM_MSB = 0xFDU;
constexpr uint8_t SENSOR_REG_HUM_LSB = 0xFEU;
constexpr uint8_t SENSOR_REG_HUM_XLSB = 0xFFU;

/* Calibration register addresses */
constexpr uint8_t SENSOR_REG_CALIB_START1 = 0x88U; // dig_T1..dig_P9 (24 bytes)
constexpr uint8_t SENSOR_REG_CALIB_START2 = 0xA1U; // dig_H1 (1 byte)
constexpr uint8_t SENSOR_REG_CALIB_START3 = 0xE1U; // dig_H2..dig_H6 (7 bytes)

/* Expected chip ID */
constexpr uint8_t SENSOR_CHIP_ID = 0x60U;

/* SPI frame lengths: 1 address byte + N data bytes */
constexpr uint8_t SPI_MEAS_FRAME_LEN = 9U;    // addr + 8 bytes (P[3]+T[3]+H[2])
constexpr uint8_t SPI_CALIB1_FRAME_LEN = 25U; // addr + 24 bytes (T1-P9)
constexpr uint8_t SPI_CALIB2_FRAME_LEN = 2U;  // addr + 1 byte  (H1)
constexpr uint8_t SPI_CALIB3_FRAME_LEN = 8U;  // addr + 7 bytes (H2-H6)

/********************************* TYPEDEFS **********************************/

enum SensorSampling : uint8_t
{
    SAMPLING_NONE = 0,
    SAMPLING_X1,
    SAMPLING_X2,
    SAMPLING_X4,
    SAMPLING_X8,
    SAMPLING_X16
};

enum class SensorMode : uint8_t
{
    SLEEP = 0,
    FORCED,
    NORMAL
};

enum SensorFilter : uint8_t
{
    FILTER_OFF = 0,
    FILTER_X2,
    FILTER_X4,
    FILTER_X8,
    FILTER_X16
};

enum class StandbyDuration : uint8_t
{
    STANDBY_0_5_MS = 0,
    STANDBY_62_5_MS,
    STANDBY_125_MS,
    STANDBY_250_MS,
    STANDBY_500_MS,
    STANDBY_1000_MS,
    STANDBY_10_MS,
    STANDBY_20_MS
};

struct SamplingConfig
{
    SensorMode Mode;
    SensorFilter FilterMode;
    StandbyDuration StdbyDuration;
    SensorSampling TemperatureSamp;
    SensorSampling HumiditySamp;
    SensorSampling PressureSamp;
};

struct Bme280CalibData
{
    /* Temperature trimming */
    uint16_t dig_T1 = 0U;
    int16_t dig_T2 = 0;
    int16_t dig_T3 = 0;
    /* Pressure trimming */
    uint16_t dig_P1 = 0U;
    int16_t dig_P2 = 0;
    int16_t dig_P3 = 0;
    int16_t dig_P4 = 0;
    int16_t dig_P5 = 0;
    int16_t dig_P6 = 0;
    int16_t dig_P7 = 0;
    int16_t dig_P8 = 0;
    int16_t dig_P9 = 0;
    /* Humidity trimming */
    uint8_t dig_H1 = 0U;
    int16_t dig_H2 = 0;
    uint8_t dig_H3 = 0U;
    int16_t dig_H4 = 0;
    int16_t dig_H5 = 0;
    int8_t dig_H6 = 0;
};

} //namespace BME280
