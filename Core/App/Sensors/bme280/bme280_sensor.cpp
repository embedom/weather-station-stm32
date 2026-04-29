/**
 ******************************************************************************
 * @file        : bme280_sensor.cpp
 * @author      : embedom
 * @date        : 2026-04-16
 * @brief       : BME280 sensor class.
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <cstdint>
#include <cmath>
#include "bme280_sensor.hpp"
#include "terminal.h"

namespace BME280
{

/******************************** CONSTEXPR **********************************/

/* CS pin: GPIOB pin 9 (active low) */
GPIO_TypeDef *const BME280_CS_PORT = GPIOB;
constexpr uint16_t BME280_CS_PIN = GPIO_PIN_9;

/* DMA transfer timeout */
constexpr TickType_t DMA_TIMEOUT_TICKS = pdMS_TO_TICKS(100U);

/* SPI protocol constants */
constexpr uint8_t DUMMY_BYTE = 0xFFU;         // Dummy byte clocked during DMA read
constexpr uint8_t SPI_WRITE_BIT_MASK = 0x7FU; // Clear bit7 to select write mode

constexpr uint8_t CONFIG_BUFFER_SIZE = 6U;

/* Calibration byte offsets within group-1 RX buffer (skip leading dummy byte) */
constexpr uint8_t C1_DIG_T1 = 0U;
constexpr uint8_t C1_DIG_T2 = 2U;
constexpr uint8_t C1_DIG_T3 = 4U;
constexpr uint8_t C1_DIG_P1 = 6U;
constexpr uint8_t C1_DIG_P2 = 8U;
constexpr uint8_t C1_DIG_P3 = 10U;
constexpr uint8_t C1_DIG_P4 = 12U;
constexpr uint8_t C1_DIG_P5 = 14U;
constexpr uint8_t C1_DIG_P6 = 16U;
constexpr uint8_t C1_DIG_P7 = 18U;
constexpr uint8_t C1_DIG_P8 = 20U;
constexpr uint8_t C1_DIG_P9 = 22U;

/* Calibration byte offsets within group-3 RX buffer (skip leading dummy byte) */
constexpr uint8_t C3_DIG_H2 = 0U;
constexpr uint8_t C3_DIG_H3 = 2U;
constexpr uint8_t C3_DIG_H4_M = 3U; // H4 most-significant nibbles
constexpr uint8_t C3_DIG_H4H5 = 4U; // shared byte: H4[3:0] | H5[3:0]
constexpr uint8_t C3_DIG_H5_M = 5U; // H5 most-significant nibbles
constexpr uint8_t C3_DIG_H6 = 6U;

/* Bosch BME280 humidity compensation constants (datasheet §4.2.3) */
constexpr int32_t HUM_TF_OFFSET = 76800;
constexpr int32_t HUM_SHIFT_14 = 14;
constexpr int32_t HUM_SHIFT_20 = 20;
constexpr int32_t HUM_ADD_16384 = 16384;
constexpr int32_t HUM_SHIFT_15 = 15;
constexpr int32_t HUM_SHIFT_10A = 10;
constexpr int32_t HUM_SHIFT_11 = 11;
constexpr int32_t HUM_ADD_32768 = 32768;
constexpr int32_t HUM_SHIFT_10B = 10;
constexpr int32_t HUM_ADD_2097152 = 2097152;
constexpr int32_t HUM_SHIFT_14B = 14;
constexpr int32_t HUM_ADD_8192 = 8192;
constexpr int32_t HUM_SHIFT_15B = 15;
constexpr int32_t HUM_SHIFT_7 = 7;
constexpr int32_t HUM_MAX = 419430400;
constexpr int32_t HUM_SHIFT_12 = 12;
constexpr int32_t HUM_SCALE = 1024;

/* Bosch BME280 pressure compensation constants (datasheet §4.2.3) */
constexpr int64_t PRES_TF_OFFSET = 128000LL;
constexpr int32_t PRES_SHIFT_17 = 17;
constexpr int32_t PRES_SHIFT_35 = 35;
constexpr int32_t PRES_SHIFT_8 = 8;
constexpr int32_t PRES_SHIFT_12 = 12;
constexpr int64_t PRES_VAR1_BASE = INT64_C(0x800000000000);
constexpr int32_t PRES_SHIFT_33 = 33;
constexpr int64_t PRES_ADC_BASE = 1048576LL;
constexpr int32_t PRES_SHIFT_31 = 31;
constexpr int64_t PRES_FACTOR = 3125LL;
constexpr int32_t PRES_SHIFT_13 = 13;
constexpr int32_t PRES_SHIFT_25 = 25;
constexpr int32_t PRES_SHIFT_19 = 19;
constexpr int32_t PRES_SHIFT_8B = 8;
constexpr int32_t PRES_SHIFT_4 = 4;
constexpr uint32_t PRES_Q248_SCALE = 256; // Q24.8 fixed-point denominator

/* Barometric formula constants for sea-level pressure conversion */
constexpr float BARO_SCALE_M = 44330.0f; // [m]
constexpr float BARO_EXPONENT = 5.255f;

constexpr uint32_t DECIMAL_SCALE = 100;

/* H4/H5 nibble masks */
constexpr uint8_t NIBBLE_LO_MASK = 0x0FU;
constexpr uint8_t NIBBLE_HI_SHIFT = 4U;
constexpr uint8_t CALIB_BYTE_SHIFT = 8U;
constexpr uint16_t CALIB_H_SIGN_BIT = 0x0800U;
constexpr uint16_t CALIB_H_SIGN_EXT_MASK = 0xF000U;

static int16_t signExtend12(uint16_t Value)
{
    if((Value & CALIB_H_SIGN_BIT) != 0U)
    {
        Value |= CALIB_H_SIGN_EXT_MASK;
    }
    return static_cast<int16_t>(Value);
}

// clang-format off
/* Pre-prepared measurement read frame */
constexpr uint8_t MEASUREMENT_TX_STATIC_BUFFER[SPI_MEAS_FRAME_LEN] = {
    SENSOR_REG_PRESS_MSB, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE,
    DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE
};

/* Pre-prepared calibration read frames */
constexpr uint8_t CALIBRATION_FIRST_REGION_TX_BUFFER[SPI_CALIB1_FRAME_LEN] = {
    SENSOR_REG_CALIB_START1, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE,
    DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE,
    DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE,
    DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE,
    DUMMY_BYTE, DUMMY_BYTE
};

constexpr uint8_t CALIBRATION_SECOND_REGION_TX_BUFFER[SPI_CALIB2_FRAME_LEN] = {
    SENSOR_REG_CALIB_START2, DUMMY_BYTE
};

constexpr uint8_t CALIBRATION_THIRD_REGION_TX_BUFFER[SPI_CALIB3_FRAME_LEN] = {
    SENSOR_REG_CALIB_START3, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE,
    DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE
};
// clang-format on

/********************************** PUBLIC ***********************************/

Status Bme280Sensor::initialize()
{
    Status InitStatus = Status::OK;
    if(_SensorState == UNINITIALIZED)
    {
        HW_Status_t Status = HW_BME280_SpiInit(&_SpiHandle, &_SpiDmaRxHandle, &_SpiDmaTxHandle);
        if(Status != HW_STATUS_ERROR)
        {
            setInterruptCallbacksCfg(&_SpiHandle, &_SpiDmaRxHandle, &_SpiDmaTxHandle, this);
            TERMINAL_LOG_INFO("BME280", "Sensor initialized (Normal mode)\n");
            _SensorState = CALIBRATION_NEED;
        }
        else
        {
            TERMINAL_LOG_ERROR("BME280", "SPI initialization failed\n");
            InitStatus = Status::CRITICAL_ERROR;
        }
    }
    return InitStatus;
}

Status Bme280Sensor::readCalibrationAndConfigure()
{
    Status CalibStatus = Status::OK;
    CalibStatus = readCalibration();
    if(CalibStatus == Status::OK)
    {
        CalibStatus = configureSensor();
        if(CalibStatus == Status::OK)
        {
            TERMINAL_LOG_INFO("BME280", "Sensor configuration successful\n");
            _SensorState = INITIALIZED;
        }
    }
    return CalibStatus;
}

void Bme280Sensor::readMeasurements()
{
    if(_SensorState == INITIALIZED)
    {
        bool TransferStatus =
            sendFrameBlocking(MEASUREMENT_TX_STATIC_BUFFER, _MeasureRxBuf, SPI_MEAS_FRAME_LEN);
        if(TransferStatus)
        {
            const int32_t PreasureRawAdc =
                static_cast<int32_t>((static_cast<uint32_t>(_MeasureRxBuf[1]) << 12U) |
                                     (static_cast<uint32_t>(_MeasureRxBuf[2]) << 4U) |
                                     (static_cast<uint32_t>(_MeasureRxBuf[3]) >> 4U));

            const int32_t TempRawAdc =
                static_cast<int32_t>((static_cast<uint32_t>(_MeasureRxBuf[4]) << 12U) |
                                     (static_cast<uint32_t>(_MeasureRxBuf[5]) << 4U) |
                                     (static_cast<uint32_t>(_MeasureRxBuf[6]) >> 4U));

            const int32_t HumidityRawAdc =
                static_cast<int32_t>((static_cast<uint32_t>(_MeasureRxBuf[7]) << 8U) |
                                     static_cast<uint32_t>(_MeasureRxBuf[8]));

            /* Temperature must be computed first — it sets _TempFine used by Humidity and Preasure */
            _Temperature = convertTemperature(TempRawAdc);
            _Humidity = convertHumidity(HumidityRawAdc);
            _Pressure = convertPressure(PreasureRawAdc);
        }
        else
        {
            TERMINAL_LOG_WARN("BME280", "Measurement read failed\n");
            return;
        }
    }
}

void Bme280Sensor::setAltitude(uint32_t AltitudeMeters)
{
    _AltitudeMeters = AltitudeMeters;
}

int32_t Bme280Sensor::getTemperature()
{
    return _Temperature;
}

uint32_t Bme280Sensor::getHumidity()
{
    return _Humidity;
}

uint32_t Bme280Sensor::getPressure()
{
    return _Pressure;
}

void Bme280Sensor::onDmaComplete()
{
    if(_TransferState == TransferState::TRANSFER_IN_PROGRESS)
    {
        _TransferState = TransferState::TRANSFER_ENDED; // Mark transfer as complete
        csDeselect();
    }
}

void Bme280Sensor::onDmaError()
{
    _TransferState = TransferState::UNEXPECTED_ERROR; // Mark transfer as failed
    csDeselect();
}

/********************************** PRIVATE **********************************/

bool Bme280Sensor::sendFrameBlocking(const uint8_t *TxBuffer, uint8_t *RxBuffer, uint16_t Length)
{
    csSelect();
    _TransferState = TransferState::TRANSFER_IN_PROGRESS;
    const HAL_StatusTypeDef Status =
        HAL_SPI_TransmitReceive_DMA(&_SpiHandle, TxBuffer, RxBuffer, Length);
    if(Status != HAL_OK)
    {
        csDeselect();
        return false;
    }
    transferSpinLock();
    return true;
}

bool Bme280Sensor::sendFrameAsync(const uint8_t *TxBuffer, uint8_t *RxBuffer, uint16_t Length)
{
    csSelect();
    const HAL_StatusTypeDef Status =
        HAL_SPI_TransmitReceive_DMA(&_SpiHandle, TxBuffer, RxBuffer, Length);
    if(Status != HAL_OK)
    {
        csDeselect();
        return false;
    }
    return true;
}

void Bme280Sensor::transferSpinLock()
{
    while(_TransferState == TransferState::TRANSFER_IN_PROGRESS)
    {
        ; // wait for complete or error callback to update state
    }
}

void Bme280Sensor::csSelect()
{
    HAL_GPIO_WritePin(BME280_CS_PORT, BME280_CS_PIN, GPIO_PIN_RESET);
}

void Bme280Sensor::csDeselect()
{
    HAL_GPIO_WritePin(BME280_CS_PORT, BME280_CS_PIN, GPIO_PIN_SET);
}

Status Bme280Sensor::readCalibration()
{
    bool TransferStatus = false;
    /* --- Group 1: 0x88..0x9F (24 bytes) → dig_T1..dig_T3, dig_P1..dig_P9 --- */
    TransferStatus =
        sendFrameBlocking(CALIBRATION_FIRST_REGION_TX_BUFFER, _CalibRxBuf, SPI_CALIB1_FRAME_LEN);
    if(TransferStatus)
    {
        const uint8_t *Calib = &_CalibRxBuf[1]; // skip dummy first byte
        _CalibrationData.dig_T1 = static_cast<uint16_t>(
            Calib[C1_DIG_T1] | (static_cast<uint16_t>(Calib[C1_DIG_T1 + 1U]) << CALIB_BYTE_SHIFT));
        _CalibrationData.dig_T2 = static_cast<int16_t>(
            Calib[C1_DIG_T2] | (static_cast<uint16_t>(Calib[C1_DIG_T2 + 1U]) << CALIB_BYTE_SHIFT));
        _CalibrationData.dig_T3 = static_cast<int16_t>(
            Calib[C1_DIG_T3] | (static_cast<uint16_t>(Calib[C1_DIG_T3 + 1U]) << CALIB_BYTE_SHIFT));
        _CalibrationData.dig_P1 = static_cast<uint16_t>(
            Calib[C1_DIG_P1] | (static_cast<uint16_t>(Calib[C1_DIG_P1 + 1U]) << CALIB_BYTE_SHIFT));
        _CalibrationData.dig_P2 = static_cast<int16_t>(
            Calib[C1_DIG_P2] | (static_cast<uint16_t>(Calib[C1_DIG_P2 + 1U]) << CALIB_BYTE_SHIFT));
        _CalibrationData.dig_P3 = static_cast<int16_t>(
            Calib[C1_DIG_P3] | (static_cast<uint16_t>(Calib[C1_DIG_P3 + 1U]) << CALIB_BYTE_SHIFT));
        _CalibrationData.dig_P4 = static_cast<int16_t>(
            Calib[C1_DIG_P4] | (static_cast<uint16_t>(Calib[C1_DIG_P4 + 1U]) << CALIB_BYTE_SHIFT));
        _CalibrationData.dig_P5 = static_cast<int16_t>(
            Calib[C1_DIG_P5] | (static_cast<uint16_t>(Calib[C1_DIG_P5 + 1U]) << CALIB_BYTE_SHIFT));
        _CalibrationData.dig_P6 = static_cast<int16_t>(
            Calib[C1_DIG_P6] | (static_cast<uint16_t>(Calib[C1_DIG_P6 + 1U]) << CALIB_BYTE_SHIFT));
        _CalibrationData.dig_P7 = static_cast<int16_t>(
            Calib[C1_DIG_P7] | (static_cast<uint16_t>(Calib[C1_DIG_P7 + 1U]) << CALIB_BYTE_SHIFT));
        _CalibrationData.dig_P8 = static_cast<int16_t>(
            Calib[C1_DIG_P8] | (static_cast<uint16_t>(Calib[C1_DIG_P8 + 1U]) << CALIB_BYTE_SHIFT));
        _CalibrationData.dig_P9 = static_cast<int16_t>(
            Calib[C1_DIG_P9] | (static_cast<uint16_t>(Calib[C1_DIG_P9 + 1U]) << CALIB_BYTE_SHIFT));

        TransferStatus = sendFrameBlocking(
            CALIBRATION_SECOND_REGION_TX_BUFFER, _CalibRxBuf, SPI_CALIB2_FRAME_LEN);
    }
    /* --- H1: 0xA1 (1 byte) --- */
    if(TransferStatus)
    {
        _CalibrationData.dig_H1 = _CalibRxBuf[1]; // skip dummy first byte
        TransferStatus = sendFrameBlocking(
            CALIBRATION_THIRD_REGION_TX_BUFFER, _CalibRxBuf, SPI_CALIB3_FRAME_LEN);
    }
    /* --- Group 3: 0xE1..0xE7 (7 bytes) → dig_H2..dig_H6 --- */
    if(TransferStatus)
    {
        const uint8_t *Calib = &_CalibRxBuf[1]; // skip dummy first byte
        _CalibrationData.dig_H2 = static_cast<int16_t>(
            Calib[C3_DIG_H2] | (static_cast<uint16_t>(Calib[C3_DIG_H2 + 1U]) << CALIB_BYTE_SHIFT));
        _CalibrationData.dig_H3 = Calib[C3_DIG_H3];
        _CalibrationData.dig_H4 =
            signExtend12((static_cast<uint16_t>(Calib[C3_DIG_H4_M]) << NIBBLE_HI_SHIFT) |
                         (Calib[C3_DIG_H4H5] & NIBBLE_LO_MASK));
        _CalibrationData.dig_H5 =
            signExtend12((static_cast<uint16_t>(Calib[C3_DIG_H5_M]) << NIBBLE_HI_SHIFT) |
                         (Calib[C3_DIG_H4H5] >> NIBBLE_HI_SHIFT));
        _CalibrationData.dig_H6 = static_cast<int8_t>(Calib[C3_DIG_H6]);
    }
    return TransferStatus ? Status::OK : Status::COMM_FAIL;
}

Status Bme280Sensor::configureSensor()
{
    bool TransferStatus = false;
    uint8_t TxBuffer[CONFIG_BUFFER_SIZE];
    uint8_t *TxBufPointer = TxBuffer;
    prepareConfigTxBuffer(TxBuffer);

    TransferStatus = sendFrameBlocking(TxBufPointer, _MeasureRxBuf, 2U);
    if(TransferStatus)
    {
        TxBufPointer += 2U;
        TransferStatus = sendFrameBlocking(TxBufPointer, _MeasureRxBuf, 2U);
    }
    if(TransferStatus)
    {
        TxBufPointer += 2U;
        TransferStatus = sendFrameBlocking(TxBufPointer, _MeasureRxBuf, 2U);
    }
    return TransferStatus ? Status::OK : Status::COMM_FAIL;
}

void Bme280Sensor::prepareConfigTxBuffer(uint8_t *TxBuffer)
{
    // NOLINTBEGIN
    TxBuffer[0] = SENSOR_REG_CTRL_HUM & SPI_WRITE_BIT_MASK;
    TxBuffer[1] = static_cast<uint8_t>(_SampConfig.HumiditySamp);
    TxBuffer[2] = SENSOR_REG_CONFIG & SPI_WRITE_BIT_MASK;
    TxBuffer[3] = static_cast<uint8_t>((static_cast<uint8_t>(_SampConfig.StdbyDuration) << 5U) |
                                       (static_cast<uint8_t>(_SampConfig.FilterMode) << 2U));
    TxBuffer[4] = SENSOR_REG_CTRL_MEAS & SPI_WRITE_BIT_MASK;
    TxBuffer[5] = static_cast<uint8_t>(
        (static_cast<uint8_t>(_SampConfig.TemperatureSamp) << 5U) |
        (static_cast<uint8_t>(_SampConfig.PressureSamp) << 2U) |
        /* SensorMode::NORMAL enum = 2, register value = 3 (0b11) */
        ((_SampConfig.Mode == SensorMode::NORMAL) ? 3U : static_cast<uint8_t>(_SampConfig.Mode)));
    // NOLINTEND
}

// clang-format off
int32_t Bme280Sensor::convertTemperature(int32_t TempRawAdc)
{
    /* Bosch BME280 datasheet compensate_T_int32 */
    const int32_t TempFirstPart =
        ((((TempRawAdc >> 3) - (static_cast<int32_t>(_CalibrationData.dig_T1) << 1))) *
         static_cast<int32_t>(_CalibrationData.dig_T2)) >> 11;

    const int32_t TempScndPart =
        (((((TempRawAdc >> 4) - static_cast<int32_t>(_CalibrationData.dig_T1)) *
           ((TempRawAdc >> 4) - static_cast<int32_t>(_CalibrationData.dig_T1))) >> 12) *
         static_cast<int32_t>(_CalibrationData.dig_T3)) >> 14;

    _TempFine = TempFirstPart + TempScndPart;
    const int32_t Temperature = (_TempFine * 5 + 128) >> 8; // 0.01 °C resolution
    return Temperature;
}

uint32_t Bme280Sensor::convertHumidity(int32_t HumidityRawAdc)
{
    /* Bosch BME280 datasheet compensate_H_int32 */
    int32_t Humidity = _TempFine - static_cast<int32_t>(HUM_TF_OFFSET);

    Humidity = (((((HumidityRawAdc << HUM_SHIFT_14) -
            (static_cast<int32_t>(_CalibrationData.dig_H4) << HUM_SHIFT_20) -
            (static_cast<int32_t>(_CalibrationData.dig_H5) * Humidity)) + HUM_ADD_16384) >> HUM_SHIFT_15) *
         (((((((Humidity * static_cast<int32_t>(_CalibrationData.dig_H6)) >> HUM_SHIFT_10A) *
              (((Humidity * static_cast<int32_t>(_CalibrationData.dig_H3)) >> HUM_SHIFT_11) +
               HUM_ADD_32768)) >> HUM_SHIFT_10B) + HUM_ADD_2097152) *
               static_cast<int32_t>(_CalibrationData.dig_H2) + HUM_ADD_8192) >> HUM_SHIFT_14B));

    Humidity -= (((((Humidity >> HUM_SHIFT_15B) * (Humidity >> HUM_SHIFT_15B)) >> HUM_SHIFT_7) *
           static_cast<int32_t>(_CalibrationData.dig_H1)) >> NIBBLE_HI_SHIFT);

    /* Validate edge values */
    Humidity = (Humidity < 0) ? 0 : Humidity;
    Humidity = (Humidity > HUM_MAX) ? HUM_MAX : Humidity;
    return (static_cast<uint32_t>((Humidity >> HUM_SHIFT_12) * DECIMAL_SCALE)) / HUM_SCALE; // [%RH]
}

uint32_t Bme280Sensor::convertPressure(int32_t PreasureRawAdc)
{
    /* Bosch BME280 datasheet compensate_P_int64 */
    int64_t Var1 = static_cast<int64_t>(_TempFine) - PRES_TF_OFFSET;
    int64_t Var2 = Var1 * Var1 * static_cast<int64_t>(_CalibrationData.dig_P6);
    Var2 += (Var1 * static_cast<int64_t>(_CalibrationData.dig_P5)) << PRES_SHIFT_17;
    Var2 += static_cast<int64_t>(_CalibrationData.dig_P4) << PRES_SHIFT_35;
    Var1 = ((Var1 * Var1 * static_cast<int64_t>(_CalibrationData.dig_P3)) >> PRES_SHIFT_8) +
           ((Var1 * static_cast<int64_t>(_CalibrationData.dig_P2)) << PRES_SHIFT_12);
    Var1 = ((PRES_VAR1_BASE + Var1) * static_cast<int64_t>(_CalibrationData.dig_P1)) >> PRES_SHIFT_33;

    if(Var1 == 0LL) return 0; // avoid division by zero

    int64_t Pressure = PRES_ADC_BASE - static_cast<int64_t>(PreasureRawAdc);
    Pressure = (((Pressure << PRES_SHIFT_31) - Var2) * PRES_FACTOR) / Var1;

    Var1 = (static_cast<int64_t>(_CalibrationData.dig_P9) * (Pressure >> PRES_SHIFT_13) *
            (Pressure >> PRES_SHIFT_13)) >> PRES_SHIFT_25;
    Var2 = (static_cast<int64_t>(_CalibrationData.dig_P8) * Pressure) >> PRES_SHIFT_19;
    Pressure = ((Pressure + Var1 + Var2) >> PRES_SHIFT_8B) +
        (static_cast<int64_t>(_CalibrationData.dig_P7) << PRES_SHIFT_4);

    /* Pressure is in Q24.8 format (Pa × 256) */
    const uint32_t PressureHpa = static_cast<uint32_t>(Pressure) / PRES_Q248_SCALE;
    // /* Convert to sea-level pressure using the barometric formula */
    // if(_AltitudeMeters > 0U)
    // {
    //     return PressureHpa / powf(1.0f - (_AltitudeMeters / BARO_SCALE_M), BARO_EXPONENT);
    // }
    return PressureHpa;
}
// clang-format on

} //namespace BME280
