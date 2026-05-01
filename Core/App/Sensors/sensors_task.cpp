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
            AppCom::TemperaturePayload DS18B20Payload = {};
            DS18B20Payload.Sequence = Sequence++;
            DS18B20Payload.TimestampTicks = xTaskGetTickCount();
            for(uint8_t SensorIndex = 0U; SensorIndex < NUMBER_OF_DS18B20_SENSORS; SensorIndex++)
            {
                const int16_t Temperature = _DS18B20TempSensor.getTemperature(SensorIndex);
                const int16_t TempIntegerPart = static_cast<int16_t>(Temperature / 100);
                const uint16_t TempFractionalPart = static_cast<uint16_t>(Temperature % 100);
                TERMINAL_LOG_DEBUG(
                    "SensorsTask", "DS18B20 Temp: %d.%02u C", TempIntegerPart, TempFractionalPart);
                DS18B20Payload.TempCeslius[SensorIndex] = Temperature;
            }
            _ItcManager.publishMessage(
                AppCom::ItcChannel::Temperature, &DS18B20Payload, sizeof(DS18B20Payload));
            /* Start another measurement sequence */
            _DS18B20TempSensor.startMeasure();
        }
        vTaskDelayUntil(&LastTimeWake, pdMS_TO_TICKS(SENSOR_TASK_CYCLE_TIME_MS));
    }
}

/********************************* PRIVATE ***********************************/

} // namespace Sensor