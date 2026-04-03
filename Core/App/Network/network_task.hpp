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
    static NetworkTask& getInstance();

    /* Remove copy constructor and assignment operator */
    NetworkTask(const NetworkTask&) = delete;
    NetworkTask& operator=(const NetworkTask&) = delete;

    void initNetwork();
    
    private:
    NetworkTask() = default;
    ~NetworkTask() = default;
    
    virtual void runCyclic() override final;
    virtual void onTaskStartUp() override final;
    void networkInit();
    bool sendTemperaturePayload(const AppCom::TemperaturePayload& payload);

    StackType_t _TaskStack[NETWORK_STACK_SIZE_WORDS];
    StaticTask_t _TaskControlBlock;

    struct netif _NetworkInterface;
    AppCom::ItcManager& _ItcManager = AppCom::ItcManager::getInstance();
    
}; //class NetworkTask

} // namespace Network