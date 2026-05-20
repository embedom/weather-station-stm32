/**
 *****************************************************************************
 * @file        : hardware_config.h
 * @author      : embedom
 * @date        : 2026-03-07
 * @brief       : Header file for board (STM32F7) hardware config.
 *****************************************************************************
 */

#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

/********************************* INCLUDES **********************************/

#include "stm32f7xx_hal.h"

/********************************** DEFINES **********************************/

#define DS18B20_UART      USART6

/********************************* TYPEDEFS **********************************/

typedef enum
{
    HW_STATUS_OK = 0,
    HW_STATUS_ERROR = -1
} HW_Status_t;

/**************************** FUNCTION PROTOTYPES ****************************/

HW_Status_t HW_ETH_Init(ETH_HandleTypeDef *EthHandle, ETH_TxPacketConfig *TxConfig,
                        ETH_DMADescTypeDef *DMATxDscrTab, ETH_DMADescTypeDef *DMARxDscrTab,
                        uint8_t *MACAddr);

HW_Status_t HW_ETH_update_mac_config(ETH_HandleTypeDef *EthHandle, uint32_t DuplexMode,
                                     uint32_t Speed);

HW_Status_t HW_DS18B20_UartInit(UART_HandleTypeDef *UartHandle, DMA_HandleTypeDef *RxDmaHandle,
                                DMA_HandleTypeDef *TxDmaHandle);

HW_Status_t HW_DS18B20_UartDeInit(UART_HandleTypeDef *UartHandle, DMA_HandleTypeDef *RxDmaHandle,
                                  DMA_HandleTypeDef *TxDmaHandle);

HW_Status_t HW_DS18B20_TimerInit(TIM_HandleTypeDef *TimerHandle);
HW_Status_t HW_systemClockConfig(void);
HW_Status_t HW_disableCache(void);
HW_Status_t HW_MpuConfig(void);

#ifdef __cplusplus
}
#endif

#endif /* HARDWARE_CONFIG_H */