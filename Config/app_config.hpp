/**
 *****************************************************************************
 * @file        : app_config.hpp
 * @author      : embedom
 * @date        : 2026-03-07
 * @brief       : General application configuration header.
 *****************************************************************************
 */

#pragma once

/********************************* INCLUDES **********************************/

#include <cstdint>

/******************************** CONSTEXPR **********************************/

constexpr uint8_t NETWORK_TASK_PRIORITY = 4U;
constexpr uint16_t NETWORK_STACK_SIZE_WORDS = 512U;
constexpr uint16_t NETWORK_TASK_CYCLE_TIME_MS = 5000U;
constexpr const char *NETWORK_TASK_NAME = "Network Task";

constexpr char NETWORK_DEVICE_IP_ADDR[] = "192.168.1.240";
constexpr char NETWORK_DEVICE_NETMASK[] = "255.255.255.0";
constexpr char NETWORK_DEVICE_GATEWAY[] = "192.168.1.1";

constexpr char HTTP_SERVER_HOST[] = "192.168.1.200";
constexpr uint16_t HTTP_SERVER_PORT = 8080;
constexpr char USER_AGENT_STRING[] = "weather-station-mcu-client/1.0";

/**********************************************************/

constexpr uint8_t SENSOR_TASK_PRIORITY = 2U;
constexpr uint16_t SENSOR_STACK_SIZE_WORDS = 256U;
constexpr uint16_t SENSOR_TASK_CYCLE_TIME_MS = 2000U;
constexpr const char *SENSOR_TASK_NAME = "Sensor com Task";

constexpr uint8_t NUMBER_OF_DS18B20_SENSORS = 1U;
constexpr uint8_t NUMBER_OF_BME280_SENSORS = 1U;

/************************* TYPEDEFS & ENUMS **********************************/

/* If defined, enables ROM search functionality for DS18B20 sensors */
// #define ENABLE_DS18B20_ROM_SEARCH

/* If defined, enables debug output for HTTP response parser */
#define HTTP_RESPONSE_PARSER_DEBUG
