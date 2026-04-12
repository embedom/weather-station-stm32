/**
 ******************************************************************************
 * @file        : itc_manager.cpp
 * @author      : embedom
 * @date        : 2026-03-21
 * @brief       : Inter task communication manager class.
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include "itc_manager.hpp"

namespace AppCom
{

/********************************** PUBLIC ***********************************/

ItcManager &ItcManager::getInstance()
{
    static ItcManager Instance;
    return Instance;
}

bool ItcManager::initialize(const ChannelConfig *Configs, size_t ConfigCount)
{
    if((!_Initialized) && (Configs != nullptr) && (ConfigCount != 0U))
    {
        for(size_t ConfigIndex = 0U; ConfigIndex < ConfigCount; ++ConfigIndex)
        {
            if(!applyChannelConfig(Configs[ConfigIndex]))
            {
                configASSERT(false);
            }
        }
        _Initialized = true;
    }
    return _Initialized;
}

bool ItcManager::publishMessage(ItcChannel Channel, const void *Data, size_t DataSize)
{
    bool IsPublished = false;
    if(_Initialized && isChannelValid(Channel))
    {
        const ItcQueueSlot &QueueSlot = _Queues[static_cast<size_t>(Channel)];
        if((QueueSlot.QueueHandle != nullptr) && QueueSlot.Configured)
        {
            if(DataSize == QueueSlot.PayloadSize)
            {
                if(QueueSlot.QueueLength == ITC_SINGLE_QUEUE_LENGTH)
                {
                    IsPublished = (xQueueOverwrite(QueueSlot.QueueHandle, Data) == pdPASS);
                }
                else
                {
                    IsPublished = (xQueueSend(QueueSlot.QueueHandle, Data, 0U) == pdPASS);
                }
            }
        }
    }
    return IsPublished;
}

bool ItcManager::waitForMessage(ItcChannel Channel, void *DataOut, size_t DataOutMaxSize,
                                TickType_t TimeoutTicks)
{
    bool IsReceived = false;
    if(_Initialized && isChannelValid(Channel) && (DataOutMaxSize != 0U))
    {
        const ItcQueueSlot &QueueSlot = _Queues[static_cast<size_t>(Channel)];
        if((QueueSlot.QueueHandle != nullptr) && QueueSlot.Configured)
        {
            if(DataOutMaxSize >= QueueSlot.PayloadSize)
            {
                if(xQueueReceive(QueueSlot.QueueHandle, DataOut, TimeoutTicks) == pdPASS)
                {
                    IsReceived = true;
                }
            }
        }
    }
    return IsReceived;
}

bool ItcManager::isChannelValid(const ItcChannel Channel)
{
    return (Channel < ItcChannel::MaxNumber);
}

bool ItcManager::applyChannelConfig(const ChannelConfig &Config)
{
    bool IsApplied = false;
    if(isChannelValid(Config.Channel) && isChannelConfigValid(Config))
    {
        ItcQueueSlot &QueueSlot = _Queues[static_cast<size_t>(Config.Channel)];
        if(!QueueSlot.Configured)
        {
            QueueSlot.QueueHandle = xQueueCreateStatic(Config.QueueLength,
                                                       Config.PayloadSize,
                                                       Config.QueueStorageBuffer,
                                                       Config.QueueControlBlock);
            if(QueueSlot.QueueHandle != nullptr)
            {
                QueueSlot.QueueName =
                    "ItcChannel" + std::to_string(static_cast<unsigned int>(Config.Channel));
                vQueueAddToRegistry(QueueSlot.QueueHandle, QueueSlot.QueueName.c_str());
                QueueSlot.PayloadSize = Config.PayloadSize;
                QueueSlot.QueueLength = Config.QueueLength;
                QueueSlot.Configured = true;
                IsApplied = true;
            }
        }
    }
    return IsApplied;
}

bool ItcManager::isChannelConfigValid(const ChannelConfig &Config)
{
    bool IsValid = false;
    if((Config.PayloadSize != 0U) && (Config.QueueLength != 0U) &&
       (Config.QueueStorageBuffer != nullptr) && (Config.QueueControlBlock != nullptr))
    {
        const size_t RequiredStorageSize =
            (Config.PayloadSize * static_cast<size_t>(Config.QueueLength));
        if(Config.QueueStorageBufferSize >= RequiredStorageSize)
        {
            IsValid = true;
        }
    }
    return IsValid;
}

} // namespace AppCom
