/**
 ******************************************************************************
 * @file        : network_task.cpp
 * @author      : embedom
 * @date        : 2026-04-03
 * @brief       : 
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <stdio.h>
#include <stdint.h>
#include "hardware_config.h"
#include "misc_compiler.h"
#include "terminal.h"

#include "network_task.hpp"
#include "itc_manager.hpp"

#include "ethernetif.h"
#include "lwip/init.h"
#include "lwip/tcpip.h"
#include "lwip/netifapi.h"
#include "lwip/dhcp.h"
#include "netif/ethernet.h"

namespace Network
{

/******************************** CONSTEXPR **********************************/

constexpr TickType_t NETWORK_MESSAGE_WAIT_TICKS = pdMS_TO_TICKS(500U);
constexpr TickType_t NETWORK_LINK_UP_WAIT_TIMEOUT_TICKS = pdMS_TO_TICKS(5000U);
constexpr TickType_t NETWORK_CYCLE_TASK_TICKS = pdMS_TO_TICKS(NETWORK_TASK_CYCLE_TIME_MS);

/********************************** PUBLIC ***********************************/

NetworkTask &NetworkTask::getInstance()
{
    static NetworkTask NetTaskInstance;
    return NetTaskInstance;
}

void NetworkTask::initNetwork()
{
    const StaticTaskConfig TaskConfig = { NETWORK_TASK_NAME,
                                          NETWORK_STACK_SIZE_WORDS,
                                          NETWORK_TASK_PRIORITY,
                                          _TaskStack,
                                          &_TaskControlBlock };
    GenericTask::createStaticTask(TaskConfig);
}

void NetworkTask::onTaskStartUp()
{
    networkStackInit();
    if(!_WeatherStationApi.initialize())
    {
        TERMINAL_LOG_ERROR("NetworkTask", "Failed to initialize weather station API");
        DEBUG_BRKPT();
    }
    TERMINAL_LOG_INFO("NetworkTask", "Network interface initialized successfully");
}

void NetworkTask::runCyclic()
{
    waitForNetworkLinkUp();
    TickType_t LastTimeWake = xTaskGetTickCount();

    for(;;)
    {
        if(!isNetworkLinkUp())
        {
            TERMINAL_LOG_INFO("NetworkTask", "Network link down, waiting for cable");
            waitForNetworkLinkUp();
        }

        AppCom::DS18B20Payload Payload = {};
        bool Received = _ItcManager.waitForMessage(
            AppCom::ItcChannel::Temperature, &Payload, sizeof(Payload), NETWORK_MESSAGE_WAIT_TICKS);

        if(Received)
        {
            processTemperaturePayload(Payload);
        }
        vTaskDelayUntil(&LastTimeWake, NETWORK_CYCLE_TASK_TICKS);
    }
}

/********************************* PRIVATE ***********************************/

void NetworkTask::networkStackInit()
{
    /* start LwIP tcpip thread for RTOS mode */
    tcpip_init(nullptr, nullptr);

    // static IP address of the microcontroller
    ip4_addr_t IpAddress, Netmask, Gateway;
    ip4addr_aton(NETWORK_DEVICE_IP_ADDR, &IpAddress); // IP address
    ip4addr_aton(NETWORK_DEVICE_NETMASK, &Netmask);   // netmask
    ip4addr_aton(NETWORK_DEVICE_GATEWAY, &Gateway);   // default gateway

    /* add network interface (in the context of tcpip thread) */
    const err_t ErrStatus = netifapi_netif_add(
        &_NetworkInterface, &IpAddress, &Netmask, &Gateway, nullptr, ethernetif_init, tcpip_input);
    if(ErrStatus != ERR_OK)
    {
        TERMINAL_LOG_ERROR("NetworkTask", "netif add failed error: %d", ErrStatus);
        DEBUG_BRKPT();
    }
    netifapi_netif_set_default(&_NetworkInterface);
    netifapi_netif_set_up(&_NetworkInterface);
}

void NetworkTask::waitForNetworkLinkUp()
{
    while(!isNetworkLinkUp())
    {
        vTaskDelay(NETWORK_LINK_UP_WAIT_TIMEOUT_TICKS);
    }
    TERMINAL_LOG_INFO("NetworkTask", "Network link is up");
}

bool NetworkTask::isNetworkLinkUp()
{
    return netif_is_link_up(&_NetworkInterface);
}

void NetworkTask::processTemperaturePayload(const AppCom::DS18B20Payload &Payload)
{
    HttpResponse Response = {};
    TERMINAL_LOG_INFO("NetworkTask",
                      "Send temperature request, sequence: %lu, timestamp ticks: %lu",
                      static_cast<unsigned long>(Payload.Sequence),
                      static_cast<unsigned long>(Payload.TimestampTicks));
    if(!_WeatherStationApi.sendDS18B20Payload(Payload, Response))
    {
        TERMINAL_LOG_ERROR("NetworkTask", "Temperature request failed to start");
        return;
    }

    handleHttpResponse(Response);
}

void NetworkTask::handleHttpResponse(const HttpResponse &Response)
{
    if(Response.Status != HttpStatus::OK)
    {
        TERMINAL_LOG_ERROR("NetworkTask",
                           "HTTP request failed, status: %u",
                           static_cast<unsigned>(Response.Status));
        return;
    }

    TERMINAL_LOG_INFO("NetworkTask", "HTTP response status code: %d", Response.StatusCode);
}

} //namespace Network
