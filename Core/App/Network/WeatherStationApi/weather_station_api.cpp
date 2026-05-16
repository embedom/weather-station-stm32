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
#include "weather_station_endpoints.hpp"
#include "app_config.hpp"
#include "terminal.h"

namespace Network
{

/******************************** CONSTEXPR **********************************/

constexpr size_t TEMPERATURE_REQUEST_BODY_SIZE = 256U;

/********************************** PUBLIC ***********************************/

bool WeatherStationApi::initialize()
{
    HttpStatus Status = _HttpClient.init(HTTP_SERVER_HOST, HTTP_SERVER_PORT);
    if(Status != HttpStatus::OK)
    {
        TERMINAL_LOG_ERROR("WeatherStationApi",
                           "HTTP client initialization failed, status: %u",
                           static_cast<unsigned>(Status));
        return false;
    }
    return true;
}

bool WeatherStationApi::sendTemperatureDS18B20(const AppCom::TemperaturePayload &Payload,
                                               HttpResponse &Response)
{
    char RequestPath[HTTP_MAX_PATH_LEN] = {};
    char RequestBody[TEMPERATURE_REQUEST_BODY_SIZE] = {};

    int Written = snprintf(
        RequestPath, sizeof(RequestPath), "%s%s", SENSORS_BASE_PATH, DS18B20_TEMP_ENDPOINT);
    if((Written <= 0) || (static_cast<size_t>(Written) >= sizeof(RequestPath)))
    {
        return false;
    }

    Written = snprintf(RequestBody,
                       sizeof(RequestBody),
                       "{\"sequence\":%lu,"
                       "\"timestamp_ticks\":%lu,"
                       "\"temperature_centi_c\":",
                       static_cast<unsigned long>(Payload.Sequence),
                       static_cast<unsigned long>(Payload.TimestampTicks));

    if((Written <= 0) || (static_cast<size_t>(Written) >= sizeof(RequestBody)))
    {
        return false;
    }

    size_t Offset = static_cast<size_t>(Written);
    if(NUMBER_OF_DS18B20_SENSORS > 1U)
    {
        Written = snprintf(&RequestBody[Offset], sizeof(RequestBody) - Offset, "[");
        Offset += static_cast<size_t>(Written);
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
    }
    else
    {
        Written = snprintf(&RequestBody[Offset],
                           sizeof(RequestBody) - Offset,
                           "%d}",
                           static_cast<int>(Payload.TempCeslius[0]));
    }

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
