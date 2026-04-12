/**
 ******************************************************************************
 * @file        : app_init.cpp
 * @author      : embedom
 * @date        : 2026-03-07
 * @brief       : Simple app init with hardware and OS initialization.
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <stdio.h>
#include <stdint.h>
#include "app_init.hpp"
#include "misc_compiler.h"
#include "hardware_config.h"
#include "SEGGER_RTT.h"

#include "itc_manager.hpp"
#include "network_task.hpp"
#include "sensors_task.hpp"

namespace App
{

/********************************* DEFINES ***********************************/

constexpr uint8_t ITC_CHANNEL_COUNT = 2U;

/******************************* PRIVATE VAR *********************************/

static StaticQueue_t ITC_TEMP_QUEUE_CONTROL = {};
static uint8_t ITC_TEMPERATURE_STORAGE[sizeof(AppCom::TemperaturePayload)] = { 0U };

static StaticQueue_t ITC_GENERIC1_QUEUE_CONTROL = {};
static uint8_t ITC_GENERIC1_STORAGE[sizeof(uint32_t)] = { 0U };

/********************************* FUNCTIONS *********************************/

AppStatus platformInit(void)
{
    if(HW_MpuConfig() != HW_STATUS_OK)
    {
        DEBUG_BRKPT();
        return AppStatus::ERROR;
    }

    if(HW_disableCache() != HW_STATUS_OK)
    {
        DEBUG_BRKPT();
        return AppStatus::ERROR;
    }

    if(HW_systemClockConfig() != HW_STATUS_OK)
    {
        DEBUG_BRKPT();
        return AppStatus::ERROR;
    }
    return AppStatus::OK;
}

AppStatus initialize(void)
{
    if(HAL_Init() != HAL_OK)
    {
        DEBUG_BRKPT();
        return AppStatus::ERROR;
    }
    SEGGER_RTT_Init();

    const AppCom::ChannelConfig ItcChannelsConfig[ITC_CHANNEL_COUNT] = {
        { .Channel = AppCom::ItcChannel::Temperature,
          .PayloadSize = sizeof(AppCom::TemperaturePayload),
          .QueueLength = 1U,
          .QueueStorageBuffer = ITC_TEMPERATURE_STORAGE,
          .QueueStorageBufferSize = sizeof(ITC_TEMPERATURE_STORAGE),
          .QueueControlBlock = &ITC_TEMP_QUEUE_CONTROL },
        { .Channel = AppCom::ItcChannel::Generic1,
          .PayloadSize = sizeof(uint32_t),
          .QueueLength = 1U,
          .QueueStorageBuffer = ITC_GENERIC1_STORAGE,
          .QueueStorageBufferSize = sizeof(ITC_GENERIC1_STORAGE),
          .QueueControlBlock = &ITC_GENERIC1_QUEUE_CONTROL }
    };

    if(!AppCom::ItcManager::getInstance().initialize(ItcChannelsConfig, ITC_CHANNEL_COUNT))
    {
        SEGGER_RTT_printf(0, "ITC init failed error\n");
        DEBUG_BRKPT();
        return AppStatus::ERROR;
    }

    Network::NetworkTask::getInstance().initNetwork();
    Sensor::SensorsTask::getInstance().initSensors();

    SEGGER_RTT_printf(0, "App base init done\n");
    return AppStatus::OK;
}

NORETURN void startOs(void)
{
    SEGGER_RTT_printf(0, "App base start OS\n");
    vTaskStartScheduler();
    /* vTaskStartScheduler should never return, if it does, it's an error */
    DEBUG_BRKPT();
    for(;;);
}

} //namespace App

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) //NOLINT
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    DEBUG_BRKPT();
    for(;;);
}