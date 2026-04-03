/**
  *****************************************************************************
  * @file        : hal_tick_tim.c
  * @author      : embedom
  * @date        : 2026-03-07
  * @brief       : HAL time base implementation based on TIM6 (1 ms tick).
  *****************************************************************************
  */

/********************************* INCLUDES **********************************/

#include "stm32f7xx_hal.h"


/********************************* TYPEDEFS **********************************/

static TIM_HandleTypeDef TimerHandle;

/********************************* FUNCTIONS *********************************/

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    RCC_ClkInitTypeDef ClkConfig = {0};
    HAL_StatusTypeDef Status = HAL_OK;
    uint32_t FlashLatency = 0;
    uint32_t TimClock = 0;
    uint32_t Apb1Prescaler = 0;
    uint32_t PrescalerValue = 0;

    if (TickPriority >= (1UL << __NVIC_PRIO_BITS))
    {
        return HAL_ERROR;
    }

    __HAL_RCC_TIM6_CLK_ENABLE();
    HAL_RCC_GetClockConfig(&ClkConfig, &FlashLatency);
    Apb1Prescaler = ClkConfig.APB1CLKDivider;

    if (Apb1Prescaler == RCC_HCLK_DIV1)
    {
        TimClock = HAL_RCC_GetPCLK1Freq();
    }
    else
    {
        TimClock = 2UL * HAL_RCC_GetPCLK1Freq();
    }

    PrescalerValue = (TimClock / 1000000UL) - 1UL;

    TimerHandle.Instance = TIM6;
    TimerHandle.Init.Period = 1000UL - 1UL;
    TimerHandle.Init.Prescaler = PrescalerValue;
    TimerHandle.Init.ClockDivision = 0;
    TimerHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    TimerHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    Status = HAL_TIM_Base_Init(&TimerHandle);
    if (Status == HAL_OK)
    {
        Status = HAL_TIM_Base_Start_IT(&TimerHandle);
    }

    if (Status == HAL_OK)
    {
        HAL_NVIC_SetPriority(TIM6_DAC_IRQn, TickPriority, 0U);
        HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
        uwTickPrio = TickPriority;
        __HAL_DBGMCU_FREEZE_TIM6();
    }
    return Status;
}

void HAL_SuspendTick(void)
{
    __HAL_TIM_DISABLE_IT(&TimerHandle, TIM_IT_UPDATE);
}

void HAL_ResumeTick(void)
{
    __HAL_TIM_ENABLE_IT(&TimerHandle, TIM_IT_UPDATE);
}

void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&TimerHandle);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM6)
    {
        HAL_IncTick();
    }
}
