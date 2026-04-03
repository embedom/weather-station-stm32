/**
  *****************************************************************************
  * @file        : app_tasks_config.hpp
  * @author      : embedom
  * @date        : 2026-03-07
  * @brief       : 
  *****************************************************************************
  */

#pragma once

/********************************* INCLUDES **********************************/

#include <stdint.h>

/******************************** CONSTEXPR **********************************/

constexpr uint8_t NETWORK_TASK_PRIORITY = 4U;
constexpr uint16_t NETWORK_STACK_SIZE_WORDS = 512U;
constexpr uint16_t NETWORK_TASK_CYCLE_TIME_MS = 4000U;
constexpr const char *NETWORK_TASK_NAME = "Network Task";

constexpr uint8_t SENSOR_TASK_PRIORITY = 2U;
constexpr uint16_t SENSOR_STACK_SIZE_WORDS = 128U;
constexpr uint16_t SENSOR_TASK_CYCLE_TIME_MS = 2000U;
constexpr const char *SENSOR_TASK_NAME = "Sensor com Task";
constexpr uint8_t NUMBER_OF_SENSORS = 2U;
