/**
 ******************************************************************************
 * @file        : bme280_sensor.hpp
 * @author      : embedom
 * @date        : 2026-04-16
 * @brief       : Class implementation for BME280 sensor.
 ******************************************************************************
 */

#pragma once

/********************************* INCLUDES **********************************/

#include <cstdint>
#include <array>
#include "hardware_config.h"
#include "bme280_enumerations.hpp"
#include "FreeRTOS.h"
#include "semphr.h"

namespace BME280
{

/********************************* TYPEDEFS **********************************/

enum SensorState : uint8_t
{
    UNINITIALIZED = 0,
    CALIBRATION_NEED,
    INITIALIZED,
    NOT_READY
};

enum TransferState : uint8_t
{
    IDLE,
    TRANSFER_IN_PROGRESS,
    TRANSFER_ENDED,
    UNEXPECTED_ERROR
};

enum Status : uint8_t
{
    OK = 0,
    INVALID_PARAM,
    COMM_FAIL,
    CRITICAL_ERROR
};

/*********************************** CLASS ***********************************/

class Bme280Sensor
{
    public:
    Bme280Sensor() = default;
    ~Bme280Sensor() = default;

    /**************************** Public Members *****************************/
    Status initialize();
    Status readCalibrationAndConfigure();
    void readMeasurements();
    void setAltitude(uint32_t AltitudeMeters);

    int32_t getTemperature();
    uint32_t getHumidity();
    uint32_t getPressure();

    /* Called from ISR context */
    void onDmaComplete();
    void onDmaError();

    /**************************** Private Members ****************************/
    private:
    SensorState _SensorState = UNINITIALIZED;
    TransferState _TransferState = IDLE;
    uint8_t _SensorId = 0U;

    /* Sampling configuration: High accuracy - T×4 / H×2 / P×8 / Filter×8 / 500 ms standby */
    SamplingConfig _SampConfig = { SensorMode::NORMAL, FILTER_X8,   StandbyDuration::STANDBY_500_MS,
                                   SAMPLING_X4,        SAMPLING_X2, SAMPLING_X8 };

    /* SPI peripheral handles */
    SPI_HandleTypeDef _SpiHandle = {};
    DMA_HandleTypeDef _SpiDmaRxHandle = {};
    DMA_HandleTypeDef _SpiDmaTxHandle = {};

    /* Calibration data */
    Bme280CalibData _CalibrationData = {};
    int32_t _TempFine = 0; // Fine temperature used by humidity and pressure compensation

    uint8_t _MeasureRxBuf[SPI_MEAS_FRAME_LEN] = {};
    uint8_t _CalibRxBuf[SPI_CALIB1_FRAME_LEN] = {};

    /* Compensated measurement results */
    int32_t _Temperature = 0;     // [°C]
    uint32_t _Humidity = 0U;       // [%RH]
    uint32_t _Pressure = 0U;       // [hPa] sea-level adjusted
    uint32_t _AltitudeMeters = 0U; // [m] above sea level for pressure conversion

    /* DMA synchronization semaphore */
    SemaphoreHandle_t _DmaSemaphore = nullptr;
    StaticSemaphore_t _DmaSemaphoreBuffer = {};

    void setInterruptCallbacksCfg(SPI_HandleTypeDef *SpiHandle, DMA_HandleTypeDef *RxDmaHandle,
                                  DMA_HandleTypeDef *TxDmaHandle, Bme280Sensor *ClassInstance);

    /* SPI communication */
    bool sendFrameBlocking(const uint8_t *TxBuffer, uint8_t *RxBuffer, uint16_t Length);
    bool sendFrameAsync(const uint8_t *TxBuffer, uint8_t *RxBuffer, uint16_t Length);
    void transferSpinLock();

    /* CS pin control */
    void csSelect();
    void csDeselect();

    /* Sensor init sub-steps */
    Status readCalibration();
    Status configureSensor();
    void prepareConfigTxBuffer(uint8_t *TxBuffer);

    /* Bosch compensation formulas (integer, from BME280 datasheet) */
    int32_t convertTemperature(int32_t TempRawAdc);
    uint32_t convertHumidity(int32_t HumidityRawAdc);
    uint32_t convertPressure(int32_t PreasureRawAdc);
};

} //namespace BME280
