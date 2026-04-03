/**
 ******************************************************************************
 * @file        : itc_manager.hpp
 * @author      : embedom
 * @date        : 2026-03-21
 * @brief       : Inter task communication API.
 ******************************************************************************
 */

#pragma once

/********************************* INCLUDES **********************************/

#include <array>
#include <cstdint>
#include <string>
#include "FreeRTOS.h"
#include "queue.h"
#include "app_config.hpp"

namespace AppCom
{

/******************************** CONSTEXPR **********************************/

constexpr UBaseType_t ITC_SINGLE_QUEUE_LENGTH = 1U;

/********************************* TYPEDEFS **********************************/

struct TemperaturePayload
{
    uint32_t Sequence = 0U;
    TickType_t TimestampTicks = 0U;
    uint16_t TemperaturesRaw[NUMBER_OF_SENSORS] = {0U};
};

enum class ItcChannel : uint8_t
{
    Temperature = 0U,
    Generic1,
    MaxNumber
};

struct ChannelConfig
{
    ItcChannel Channel = ItcChannel::Temperature;
    size_t PayloadSize = 0U;
    UBaseType_t QueueLength = ITC_SINGLE_QUEUE_LENGTH;
    uint8_t* QueueStorageBuffer = nullptr;
    size_t QueueStorageBufferSize = 0U;
    StaticQueue_t* QueueControlBlock = nullptr;
};

/*********************************** CLASS ***********************************/

class ItcManager
{
    /**************************** PUBLIC MEMBERS *****************************/
    public:
    ItcManager(const ItcManager&) = delete;
    ItcManager& operator=(const ItcManager&) = delete;

    static ItcManager& getInstance();
    bool initialize(const ChannelConfig* Configs, size_t ConfigCount);
    bool publishMessage(ItcChannel Channel, const void* Data, size_t DataSize);
    bool waitForMessage(ItcChannel Channel, void* DataOut, size_t DataOutMaxSize, 
                                                        TickType_t TimeoutTicks);

    /**************************** PRIVATE MEMBERS ****************************/
    private:

    struct ItcQueueSlot
    {
        QueueHandle_t QueueHandle = nullptr;
        std::string QueueName = {};
        size_t PayloadSize = 0U;
        UBaseType_t QueueLength = 0U;
        bool Configured = false;
    };

    bool isChannelValid(const ItcChannel Channel);
    bool applyChannelConfig(const ChannelConfig& Config);
    bool isChannelConfigValid(const ChannelConfig& Config);

    ItcManager() = default;
    bool _Initialized = false;
    std::array<ItcQueueSlot, static_cast<size_t>(ItcChannel::MaxNumber)> _Queues = {};

}; // class ItcManager

} // namespace AppCom