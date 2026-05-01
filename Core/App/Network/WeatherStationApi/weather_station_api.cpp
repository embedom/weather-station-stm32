/**
 ******************************************************************************
 * @file        : weather_station_api.cpp
 * @author      : embedom
 * @date        : 2026-05-13
 * @brief       : Weather station HTTP API adapter implementation.
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <stdio.h>
#include "weather_station_api.hpp"
#include "app_config.hpp"

namespace Network
{

/******************************** CONSTEXPR **********************************/

constexpr size_t TEMPERATURE_REQUEST_BODY_SIZE = 256U;

/********************************** PUBLIC ***********************************/

bool WeatherStationApi::initialize()
{
    _HttpClient.init(HTTP_SERVER_HOST, HTTP_SERVER_PORT);
    return true;
}

bool WeatherStationApi::sendTemperature(const AppCom::TemperaturePayload &Payload,
                                        HttpResponse &Response)
{
    char RequestPath[HTTP_MAX_PATH_LEN] = {};
    char RequestBody[TEMPERATURE_REQUEST_BODY_SIZE] = {};

    int Written =
        snprintf(RequestPath, sizeof(RequestPath), "%s%s", API_BASE_PATH, TEMPERATURE_ENDPOINT);
    if((Written <= 0) || (static_cast<size_t>(Written) >= sizeof(RequestPath)))
    {
        return false;
    }

    Written = snprintf(RequestBody,
                       sizeof(RequestBody),
                       "{\"sequence\":%lu,"
                       "\"timestamp_ticks\":%lu,"
                       "\"temperatures_centi_c\":[",
                       static_cast<unsigned long>(Payload.Sequence),
                       static_cast<unsigned long>(Payload.TimestampTicks));

    if((Written <= 0) || (static_cast<size_t>(Written) >= sizeof(RequestBody)))
    {
        return false;
    }

    size_t Offset = static_cast<size_t>(Written);
    for(uint8_t SensorIndex = 0U; SensorIndex < NUMBER_OF_DS18B20_SENSORS; ++SensorIndex)
    {
        Written = snprintf(&RequestBody[Offset],
                           sizeof(RequestBody) - Offset,
                           "%s%d",
                           (SensorIndex == 0U) ? "" : ",",
                           static_cast<int>(Payload.TempCeslius[SensorIndex]));
        if((Written <= 0) || (static_cast<size_t>(Written) >= (sizeof(RequestBody) - Offset)))
        {
            return false;
        }
        Offset += static_cast<size_t>(Written);
    }

    Written = snprintf(&RequestBody[Offset], sizeof(RequestBody) - Offset, "]}");
    if((Written <= 0) || (static_cast<size_t>(Written) >= (sizeof(RequestBody) - Offset)))
    {
        return false;
    }

    bool PostResult = _HttpClient.post(RequestPath, RequestBody, Response);
    if(PostResult)
    {
        processResponseBody(Response);
    }
    return PostResult;
}

void WeatherStationApi::processResponseBody(const HttpResponse &Response)
{
    (void)Response;
    //TODO add processing of response body if needed (e.g. parse JSON with server response)
}

} // namespace Network
