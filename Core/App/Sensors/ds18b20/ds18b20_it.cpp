/**
 ******************************************************************************
 * @file        : ds18b20_it.cpp
 * @author      : embedom
 * @date        : 2026-03-28
 * @brief       : Interrupts handling for ds18b20 - uart sensor.
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <stdio.h>
#include <stdint.h>
#include "stm32f7xx_hal.h"
#include "ds18b20.hpp"
#include "hardware_config.h"

namespace DS18B20
{

/******************************** CONSTEXPR **********************************/

/***************************** PRIVATE VAR ***********************************/

static UART_HandleTypeDef *UART_HANDLE = nullptr;
static TIM_HandleTypeDef *TIMER_HANDLE = nullptr;
static DS18B20Sensor *CLASS_INSTANCE = nullptr;

/******************************** INTERRUPTS *********************************/

void DS18B20Sensor::setInterruptCallbacksCfg(DS18B20Sensor *Instance,
                                             UART_HandleTypeDef *UartHandle,
                                             TIM_HandleTypeDef *TimHandle)
{
    CLASS_INSTANCE = Instance;
    UART_HANDLE = UartHandle;
    TIMER_HANDLE = TimHandle;
}

extern "C" void DMA2_Stream2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(UART_HANDLE->hdmarx);
}

extern "C" void DMA2_Stream7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(UART_HANDLE->hdmatx);
}

extern "C" void USART6_IRQHandler(void)
{
    HAL_UART_IRQHandler(UART_HANDLE);
}

extern "C" void TIM7_IRQHandler(void)
{
    if((__HAL_TIM_GET_FLAG(TIMER_HANDLE, TIM_FLAG_UPDATE) != RESET) &&
       (__HAL_TIM_GET_IT_SOURCE(TIMER_HANDLE, TIM_IT_UPDATE) != RESET))
    {
        __HAL_TIM_CLEAR_IT(TIMER_HANDLE, TIM_IT_UPDATE);
        CLASS_INSTANCE->handleConversionTimerElapsed();
    }
}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == DS18B20_UART)
    {
        CLASS_INSTANCE->handleTransactionComplete();
    }
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == DS18B20_UART)
    {
        CLASS_INSTANCE->handleUartError();
    }
}

} //namespace DS18B20