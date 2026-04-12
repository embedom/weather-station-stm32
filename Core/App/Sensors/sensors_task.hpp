/**
 ******************************************************************************
 * @file        : sensors_task.hpp
 * @author      : embedom
 * @date        : 2026-03-28
 * @brief       : 
 ******************************************************************************
 */
#pragma once

/********************************* INCLUDES **********************************/

#include <cstdint>

#include "generic_task.hpp"
#include "app_config.hpp"
#include "itc_manager.hpp"
#include "ds18b20.hpp"

#include "FreeRTOS.h"
#include "task.h"

namespace Sensor
{

/******************************** CONSTEXPR **********************************/

/********************************* TYPEDEFS **********************************/

/*********************************** CLASS ***********************************/

class SensorsTask : public GenericTask
{
    public:
    static SensorsTask &getInstance();

    /* Remove copy constructor and assignment operator */
    SensorsTask(const SensorsTask &) = delete;
    SensorsTask &operator=(const SensorsTask &) = delete;

    void initSensors();

    private:
    SensorsTask() = default;
    ~SensorsTask() = default;

    virtual void runCyclic() override final;
    virtual void onTaskStartUp() override final;

    StackType_t _TaskStack[SENSOR_STACK_SIZE_WORDS];
    StaticTask_t _TaskControlBlock;

    DS18B20::DS18B20Sensor _TemperatureSensor = {};
    AppCom::ItcManager &_ItcManager = AppCom::ItcManager::getInstance();

}; //class SensorsTask

} //namespace Sensor
