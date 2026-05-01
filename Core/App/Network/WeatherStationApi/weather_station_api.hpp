/**
 ******************************************************************************
 * @file        : weather_station_api.hpp
 * @author      : embedom
 * @date        : 2026-05-13
 * @brief       : Weather station HTTP API adapter.
 ******************************************************************************
 */

#pragma once

/********************************* INCLUDES **********************************/

#include "http_client.hpp"
#include "itc_manager.hpp"

namespace Network
{

/*********************************** CLASS ***********************************/

class WeatherStationApi
{
    public:
    WeatherStationApi() = default;

    bool initialize();
    bool sendTemperature(const AppCom::TemperaturePayload &Payload, HttpResponse &Response);

    private:
    void processResponseBody(const HttpResponse &Response);
    static constexpr const char *API_BASE_PATH = "/api/weather-station/";
    static constexpr const char *TEMPERATURE_ENDPOINT = "temperature";

    HttpClient _HttpClient;

}; //class WeatherStationApi

} // namespace Network
