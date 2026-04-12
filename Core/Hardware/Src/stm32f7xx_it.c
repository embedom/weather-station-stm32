/**
 ******************************************************************************
 * @file        : stm32f7xx_it.c
 * @author      : embedom
 * @date        : 2026-03-07
 * @brief       : Cortex-M7 Processor Interruption and Exception Handlers.
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <stdint.h>
#include "stm32f7xx_hal.h"
#include "stm32f7xx_it.h"
#include "misc_compiler.h"

/********************************* FUNCTIONS *********************************/

void NMI_Handler(void)
{
    while(1)
    {
        DEBUG_BRKPT();
    }
}

void HardFault_Handler(void)
{
    while(1)
    {
        DEBUG_BRKPT();
    }
}

void MemManage_Handler(void)
{
    while(1)
    {
        DEBUG_BRKPT();
    }
}

void BusFault_Handler(void)
{
    while(1)
    {
        DEBUG_BRKPT();
    }
}

void UsageFault_Handler(void)
{
    while(1)
    {
        DEBUG_BRKPT();
    }
}

void DebugMon_Handler(void)
{
    DEBUG_BRKPT();
}

void Error_Handler(void)
{
    __disable_irq();
    while(1)
    {
        DEBUG_BRKPT();
    }
}