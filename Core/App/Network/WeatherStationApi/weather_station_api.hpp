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

#include "itc_manager.hpp"
#include "http_client.hpp"

namespace Network
{

/*********************************** CLASS ***********************************/

class WeatherStationApi
{
    public:
    WeatherStationApi(HttpClient &HttpClient) : _HttpClient(HttpClient) {};

    bool initialize();
    bool sendDS18B20Payload(const AppCom::DS18B20Payload &Payload, HttpResponse &Response);

    private:
    void processResponseBody(const HttpResponse &Response);

    HttpClient &_HttpClient;

}; //class WeatherStationApi

} // namespace Network
