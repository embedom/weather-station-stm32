/**
 ******************************************************************************
 * @file        : sensors_task.cpp
 * @author      : embedom
 * @date        : 2026-03-28
 * @brief       : Implementation for the sensors task.
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <stdio.h>
#include <stdint.h>
#include "misc_compiler.h"
#include "hardware_config.h"
#include "sensors_task.hpp"
#include "itc_manager.hpp"
#include "terminal.h"

namespace Sensor
{

/******************************** CONSTEXPR **********************************/

constexpr int DECIMAL_SCALE = 100;
constexpr uint32_t POLAND_AVERAGE_ALTITUDE = 170;

/********************************* TYPEDEFS **********************************/

/********************************** PUBLIC ***********************************/

SensorsTask &SensorsTask::getInstance()
{
    static SensorsTask SensorsTaskInstance;
    return SensorsTaskInstance;
}

void SensorsTask::initSensors()
{
    const StaticTaskConfig TaskConfig = { SENSOR_TASK_NAME,
                                          SENSOR_STACK_SIZE_WORDS,
                                          SENSOR_TASK_PRIORITY,
                                          _TaskStack,
                                          &_TaskControlBlock };

    GenericTask::createStaticTask(TaskConfig);
}

void SensorsTask::onTaskStartUp()
{
    _DS18B20TempSensor.initialize();
    _Bme280Sensor.initialize();
    _Bme280Sensor.readCalibrationAndConfigure();
    // _Bme280Sensor.setAltitude(POLAND_AVERAGE_ALTITUDE);
    TERMINAL_LOG_INFO("SensorsTask", "Sensor task initialized successfully");
}

void SensorsTask::runCyclic()
{
    TickType_t LastTimeWake = xTaskGetTickCount();
    uint32_t Sequence = 0U;

    for(;;)
    {
        if(_DS18B20TempSensor.isSensorReady())
        {
            AppCom::DS18B20Payload DS18B20Payload = {};
            DS18B20Payload.Sequence = Sequence++;
            DS18B20Payload.TimestampTicks = xTaskGetTickCount();
            for(uint8_t SensorIndex = 0U; SensorIndex < NUMBER_OF_DS18B20_SENSORS; SensorIndex++)
            {
                const int16_t Temperature = _DS18B20TempSensor.getTemperature(SensorIndex);
                const int16_t TempIntegerPart = static_cast<int16_t>(Temperature / DECIMAL_SCALE);
                const uint16_t TempFractionalPart =
                    static_cast<uint16_t>(Temperature % DECIMAL_SCALE);
                TERMINAL_LOG_DEBUG(
                    "SensorsTask", "DS18B20 Temp: %d.%02u C", TempIntegerPart, TempFractionalPart);
                DS18B20Payload.TempCeslius[SensorIndex] = Temperature;
            }
            _ItcManager.publishMessage(
                AppCom::ItcChannel::DS18B20, &DS18B20Payload, sizeof(DS18B20Payload));
            /* Start another measurement sequence */
            _DS18B20TempSensor.startMeasure();
        }

        /* BME280 — read burst frame via DMA (sensor runs in Normal mode) */
        _Bme280Sensor.readMeasurements();

        AppCom::Bme280Payload Bme280Payload = {};
        Bme280Payload.Sequence = Sequence;
        Bme280Payload.TimestampTicks = xTaskGetTickCount();
        Bme280Payload.Temperature = _Bme280Sensor.getTemperature();
        Bme280Payload.Humidity = _Bme280Sensor.getHumidity();
        Bme280Payload.Pressure = _Bme280Sensor.getPressure();

        TERMINAL_LOG_DEBUG("SensorsTask",
                           "BME280: T=%d.%02u C  H=%u.%02u%%  P=%u.%02u hPa",
                           static_cast<int>(Bme280Payload.Temperature / DECIMAL_SCALE),
                           static_cast<uint16_t>(Bme280Payload.Temperature % DECIMAL_SCALE),
                           static_cast<int>(Bme280Payload.Humidity / DECIMAL_SCALE),
                           static_cast<uint16_t>(Bme280Payload.Humidity % DECIMAL_SCALE),
                           static_cast<int>(Bme280Payload.Pressure / DECIMAL_SCALE),
                           static_cast<uint16_t>(Bme280Payload.Pressure % DECIMAL_SCALE));

        _ItcManager.publishMessage(
            AppCom::ItcChannel::BME280, &Bme280Payload, sizeof(Bme280Payload));

        Sequence++;
        vTaskDelayUntil(&LastTimeWake, pdMS_TO_TICKS(SENSOR_TASK_CYCLE_TIME_MS));
    }
}

/********************************* PRIVATE ***********************************/

} // namespace Sensor