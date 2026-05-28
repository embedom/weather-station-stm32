/**
 ******************************************************************************
 * @file        : weather_station_endpoints.hpp
 * @author      : embedom
 * @date        : 2026-05-16
 * @brief       : Weather station API endpoints.
 ******************************************************************************
 */

#pragma once

namespace Network
{

constexpr char API_BASE_PATH[] = "/api/weather-station/";
constexpr char API_CONFIG_PATH[] = "/api/weather-station/config/";

constexpr char SENSORS_BASE_PATH[] = "/api/weather-station/sensors/";
constexpr char DS18B20_TEMP_ENDPOINT[] = "ds18b20/measurements";
constexpr char BME280_MEASUR_ENDPOINT[] = "bme280/measurements";

constexpr char CONFIG_BASE_PATH[] = "/api/weather-station/config";
constexpr char USER_DATA_ENDPOINT[] = "/api/weather-station/user-data";

} // namespace Network
