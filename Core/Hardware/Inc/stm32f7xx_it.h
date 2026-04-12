/**
  *****************************************************************************
  * @file        : stm32f7xx_it.h
  * @author      : embedom
  * @date        : 2026-03-07
  * @brief       : 
  *****************************************************************************
  */

#ifndef STM32F7XX_IT_H
#define STM32F7XX_IT_H

#ifdef __cplusplus
extern "C"
{
#endif

    void NMI_Handler(void);
    void HardFault_Handler(void);
    void MemManage_Handler(void);
    void BusFault_Handler(void);
    void UsageFault_Handler(void);
    void DebugMon_Handler(void);

    void TIM6_DAC_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32F7XX_IT_H */