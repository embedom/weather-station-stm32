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
#include "lwip/apps/http_client.h"
#include "lwip/sockets.h"

namespace Network
{

/******************************** CONSTEXPR **********************************/

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
    networkInit();
    TERMINAL_LOG_INFO("NetworkTask", "Network interface initialized successfully");
}

void NetworkTask::runCyclic()
{
    TickType_t LastTimeWake = xTaskGetTickCount();
    for(;;)
    {
        AppCom::TemperaturePayload Payload = {};
        bool Received = _ItcManager.waitForMessage(
            AppCom::ItcChannel::Temperature, &Payload, sizeof(Payload), pdMS_TO_TICKS(1000U));

        if(Received)
        {
            // sendTemperaturePayload(Payload);
        }
        vTaskDelayUntil(&LastTimeWake, pdMS_TO_TICKS(NETWORK_TASK_CYCLE_TIME_MS));
    }
}

/********************************* PRIVATE ***********************************/

void NetworkTask::networkInit()
{
    /* start LwIP tcpip thread for RTOS mode */
    tcpip_init(nullptr, nullptr);

    // static IP address of the microcontroller
    ip4_addr_t IpAddress, Netmask, Gateway;
    IP4_ADDR(&IpAddress, 192, 168, 1, 240); // IP address
    IP4_ADDR(&Netmask, 255, 255, 255, 0);   // netmask
    IP4_ADDR(&Gateway, 192, 168, 1, 200);   // default gateway

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

} //namespace Network