/**
 ******************************************************************************
 * @file        : sensors_task.cpp
 * @author      : embedom
 * @date        : 2026-03-28
 * @brief       : 
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <stdio.h>
#include <stdint.h>
#include "misc_compiler.h"
#include "hardware_config.h"
#include "sensors_task.hpp"
#include "itc_manager.hpp"

#include "SEGGER_RTT.h"

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
    _TemperatureSensor.initialize();
    _TemperatureSensor.startMeasure();
    SEGGER_RTT_printf(0, "Sensor task initialized\n");
}

void SensorsTask::runCyclic()
{
    TickType_t LastTimeWake = xTaskGetTickCount();
    uint32_t Sequence = 0U;

    for(;;)
    {
        AppCom::TemperaturePayload Payload = {};
        Payload.Sequence = Sequence++;
        Payload.TimestampTicks = xTaskGetTickCount();
        for(uint8_t SensorIndex = 0U; SensorIndex < NUMBER_OF_SENSORS; SensorIndex++)
        {
            const uint16_t Temperature = _TemperatureSensor.getTemperature(SensorIndex);
            Payload.TemperaturesRaw[SensorIndex] = Temperature;
        }

        SEGGER_RTT_printf(0, "Sensor data: %u\n", Payload.TemperaturesRaw[0]);
        _TemperatureSensor.startMeasure();
        _ItcManager.publishMessage(AppCom::ItcChannel::Temperature, &Payload, sizeof(Payload));

        vTaskDelayUntil(&LastTimeWake, pdMS_TO_TICKS(SENSOR_TASK_CYCLE_TIME_MS)); // delay 5s
    }
}

/********************************* PRIVATE ***********************************/

} // namespace Sensor