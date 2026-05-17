/**
 ******************************************************************************
 * @file        : network_task.hpp
 * @author      : embedom
 * @date        : 2026-04-03
 * @brief       : 
 ******************************************************************************
 */

#pragma once

/********************************* INCLUDES **********************************/

#include "generic_task.hpp"
#include "app_config.hpp"
#include "itc_manager.hpp"
#include "tcp_transport.hpp"
#include "http_client.hpp"
#include "weather_station_api.hpp"

#include "FreeRTOS.h"
#include "lwip/netif.h"

namespace Network
{

/******************************** CONSTEXPR **********************************/

/********************************* TYPEDEFS **********************************/

/*********************************** CLASS ***********************************/

class NetworkTask : public GenericTask
{
    public:
    static NetworkTask &getInstance();

    /* Remove copy constructor and assignment operator */
    NetworkTask(const NetworkTask &) = delete;
    NetworkTask &operator=(const NetworkTask &) = delete;

    void initNetwork();

    private:
    NetworkTask() : _Transport(), _HttpClient(_Transport), _WeatherStationApi(_HttpClient) {};
    ~NetworkTask() = default;

    virtual void runCyclic() override final;
    virtual void onTaskStartUp() override final;
    void networkStackInit();
    void waitForNetworkLinkUp();
    bool isNetworkLinkUp();
    void processTemperaturePayload(const AppCom::DS18B20Payload &Payload);
    void handleHttpResponse(const HttpResponse &Response);

    StackType_t _TaskStack[NETWORK_STACK_SIZE_WORDS];
    StaticTask_t _TaskControlBlock;

    struct netif _NetworkInterface;
    AppCom::ItcManager &_ItcManager = AppCom::ItcManager::getInstance();

    TcpTransport _Transport;
    HttpClient _HttpClient;
    WeatherStationApi _WeatherStationApi;

}; //class NetworkTask

} // namespace Network
