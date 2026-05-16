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

    static constexpr const char *API_BASE_PATH = "/api/weather-station/";
    static constexpr const char *API_CONFIG_PATH = "/api/weather-station/config/";

    static constexpr const char *SENSORS_BASE_PATH = "/api/weather-station/sensors/";
    static constexpr const char *DS18B20_TEMP_ENDPOINT = "ds18b20/temperature";
    static constexpr const char *BME280_MEASUR_ENDPOINT = "bme280/measurements";

} // namespace ApiEndpoints
