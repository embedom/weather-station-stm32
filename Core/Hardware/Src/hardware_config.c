/**
 ******************************************************************************
 * @file        : hardware_config.c
 * @author      : embedom
 * @date        : 2026-03-07
 * @brief       : This file contains general board (STM32F7) hardware config.
 ******************************************************************************
 */


/********************************* INCLUDES **********************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "hardware_config.h"
#include "misc_compiler.h"
#include "core_cm7.h"

/********************************* DEFINES ***********************************/

#define ETH_IRQ_PRIORITY 5U
#define DS18B20_IRQ_PRIORITY 6U

/*********** RMII DEFINES ***********/
#define RMII_TXD1_Pin GPIO_PIN_14
#define RMII_TXD0_Pin GPIO_PIN_13
#define RMII_TX_EN_Pin GPIO_PIN_11
#define RMII_MDC_Pin GPIO_PIN_1
#define RMII_REF_CLK_Pin GPIO_PIN_1
#define RMII_RXD0_Pin GPIO_PIN_4
#define RMII_MDIO_Pin GPIO_PIN_2
#define RMII_RXD1_Pin GPIO_PIN_5
#define RMII_CRS_DV_Pin GPIO_PIN_7

/********************************* TYPEDEFS **********************************/



/********************************* FUNCTIONS *********************************/

void HAL_MspInit(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    /* System interrupt init*/
}

HW_Status_t HW_systemClockConfig(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    /* Configure the main internal regulator output voltage */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    if(HAL_PWREx_EnableOverDrive() != HAL_OK)
    {
        return HW_STATUS_ERROR;
    }

    /* Initializes the RCC Oscillators according to the specified parameters
        in the RCC_OscInitTypeDef structure. */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

    RCC_OscInitStruct.PLL.PLLM = 25;   // 25 MHz / 25 = 1 MHz
    RCC_OscInitStruct.PLL.PLLN = 432;  // 1 MHz * 432 = 432 MHz
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;  // 432 / 2 = 216 MHz
    RCC_OscInitStruct.PLL.PLLQ = 9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        return HW_STATUS_ERROR;
    }

    /* Initializes the CPU, AHB and APB buses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_HCLK   |
                                  RCC_CLOCKTYPE_PCLK1  |
                                  RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;   // 216 MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;     // 54 MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;     // 108 MHz

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
    {
        return HW_STATUS_ERROR;
    }
    return HW_STATUS_OK;
}

HW_Status_t HW_disableCache(void)
{
    if (SCB->CCR & SCB_CCR_DC_Msk)
    {
        SCB_DisableDCache();
    }

    if (SCB->CCR & SCB_CCR_IC_Msk)
    {
        SCB_DisableICache();
    }
    return HW_STATUS_OK;
}

HW_Status_t HW_MpuConfig(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct = {0};
    /* Disables the MPU */
    HAL_MPU_Disable();

    /* Initializes and configures the Region and the memory to be protected */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress = 0x0;
    MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
    MPU_InitStruct.SubRegionDisable = 0x87;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    /* Enables the MPU */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
    return HW_STATUS_OK;
}

/******************************* ETHERNET HW *********************************/

HW_Status_t HW_ETH_Init(ETH_HandleTypeDef *EthHandle, ETH_TxPacketConfig *TxConfig,
    ETH_DMADescTypeDef *DMATxDscrTab, ETH_DMADescTypeDef *DMARxDscrTab, uint8_t *MACAddr)
{
    EthHandle->Instance = ETH;
    MACAddr[0] = 0x00;
    MACAddr[1] = 0x80;
    MACAddr[2] = 0xE1;
    MACAddr[3] = 0x00;
    MACAddr[4] = 0x10;
    MACAddr[5] = 0x10;
    EthHandle->Init.MACAddr = MACAddr;
    EthHandle->Init.MediaInterface = HAL_ETH_RMII_MODE;
    EthHandle->Init.TxDesc = DMATxDscrTab;
    EthHandle->Init.RxDesc = DMARxDscrTab;
    EthHandle->Init.RxBuffLen = ETH_RX_BUF_SIZE;

    memset(TxConfig, 0 , sizeof(ETH_TxPacketConfig));
    TxConfig->Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
    TxConfig->ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
    TxConfig->CRCPadCtrl = ETH_CRC_PAD_INSERT;
    
    if(HAL_ETH_Init(EthHandle) != HAL_OK)
    {
        return HW_STATUS_ERROR;
    }
    return HW_STATUS_OK;
}

HW_Status_t HW_ETH_update_mac_config(ETH_HandleTypeDef *EthHandle, uint32_t DuplexMode, uint32_t Speed)
{
    ETH_MACConfigTypeDef MACConf = {0};
    /* Get MAC Config MAC */
    if(HAL_ETH_GetMACConfig(EthHandle, &MACConf) != HAL_OK)
    {
        return HW_STATUS_ERROR;
    }
    MACConf.DuplexMode = DuplexMode;
    MACConf.Speed = Speed;
    MACConf.ChecksumOffload = ENABLE; /* hardware TCP/UDP/IP checksum */
    MACConf.DropTCPIPChecksumErrorPacket = ENABLE;
    if(HAL_ETH_SetMACConfig(EthHandle, &MACConf) != HAL_OK)
    {
        return HW_STATUS_ERROR;
    }
    if(HAL_ETH_Start_IT(EthHandle) != HAL_OK)
    {
        return HW_STATUS_ERROR;
    }
    return HW_STATUS_OK;
}

void HAL_ETH_MspInit(ETH_HandleTypeDef* EthHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(EthHandle->Instance == ETH)
    {
        /* Enable Peripheral clock */
        __HAL_RCC_ETH_CLK_ENABLE();
        __HAL_RCC_GPIOG_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin = RMII_TXD1_Pin|RMII_TXD0_Pin|RMII_TX_EN_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
        HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = RMII_MDC_Pin|RMII_RXD0_Pin|RMII_RXD1_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = RMII_REF_CLK_Pin|RMII_MDIO_Pin|RMII_CRS_DV_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* Peripheral interrupt init */
        HAL_NVIC_SetPriority(ETH_IRQn, ETH_IRQ_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(ETH_IRQn);
        HAL_NVIC_SetPriority(ETH_WKUP_IRQn, ETH_IRQ_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(ETH_WKUP_IRQn);
    }
}

void HAL_ETH_MspDeInit(ETH_HandleTypeDef* EthHandle)
{
    if(EthHandle->Instance == ETH)
    {
        /* Peripheral clock disable */
        __HAL_RCC_ETH_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOG, RMII_TXD1_Pin|RMII_TXD0_Pin|RMII_TX_EN_Pin);
        HAL_GPIO_DeInit(GPIOC, RMII_MDC_Pin|RMII_RXD0_Pin|RMII_RXD1_Pin);
        HAL_GPIO_DeInit(GPIOA, RMII_REF_CLK_Pin|RMII_MDIO_Pin|RMII_CRS_DV_Pin);
        /* Peripheral interrupt Deinit*/
        HAL_NVIC_DisableIRQ(ETH_IRQn);
        HAL_NVIC_DisableIRQ(ETH_WKUP_IRQn);
    }
}

/******************************* DS18B20 HW **********************************/

HW_Status_t HW_DS18B20_UartInit(UART_HandleTypeDef *UartHandle, DMA_HandleTypeDef *RxDmaHandle, 
                                                        DMA_HandleTypeDef *TxDmaHandle)
{
    HAL_StatusTypeDef HalStatus = HAL_OK;
    GPIO_InitTypeDef GpioInit = {0};
    __HAL_RCC_USART6_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // USART6_TX on PC6 in AF8, open-drain for one-wire bus with pull-up.
    GpioInit.Pin = GPIO_PIN_6;
    GpioInit.Mode = GPIO_MODE_AF_OD;
    GpioInit.Pull = GPIO_PULLUP;
    GpioInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GpioInit.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOC, &GpioInit);

    // USART6_RX on PC7 in AF8, input for one-wire bus read-back.
    GpioInit.Pin = GPIO_PIN_7;
    GpioInit.Mode = GPIO_MODE_AF_PP;
    GpioInit.Pull = GPIO_NOPULL;
    GpioInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GpioInit.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOC, &GpioInit);

    UartHandle->Instance = DS18B20_UART;
    UartHandle->Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle->Init.StopBits = UART_STOPBITS_1;
    UartHandle->Init.Parity = UART_PARITY_NONE;
    UartHandle->Init.Mode = UART_MODE_TX_RX;
    UartHandle->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    UartHandle->Init.OverSampling = UART_OVERSAMPLING_16;

    __HAL_RCC_DMA2_CLK_ENABLE();
    RxDmaHandle->Instance = DMA2_Stream2;
    RxDmaHandle->Init.Channel = DMA_CHANNEL_5;
    RxDmaHandle->Init.Direction = DMA_PERIPH_TO_MEMORY;
    RxDmaHandle->Init.PeriphInc = DMA_PINC_DISABLE;
    RxDmaHandle->Init.MemInc = DMA_MINC_ENABLE;
    RxDmaHandle->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    RxDmaHandle->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    RxDmaHandle->Init.Mode = DMA_NORMAL;
    RxDmaHandle->Init.Priority = DMA_PRIORITY_HIGH;
    RxDmaHandle->Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    TxDmaHandle->Instance = DMA2_Stream7;
    TxDmaHandle->Init.Channel = DMA_CHANNEL_5;
    TxDmaHandle->Init.Direction = DMA_MEMORY_TO_PERIPH;
    TxDmaHandle->Init.PeriphInc = DMA_PINC_DISABLE;
    TxDmaHandle->Init.MemInc = DMA_MINC_ENABLE;
    TxDmaHandle->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    TxDmaHandle->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    TxDmaHandle->Init.Mode = DMA_NORMAL;
    TxDmaHandle->Init.Priority = DMA_PRIORITY_HIGH;
    TxDmaHandle->Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    HalStatus = HAL_DMA_Init(RxDmaHandle);
    if(HalStatus != HAL_OK)
    {
        HAL_DMA_DeInit(RxDmaHandle);
        return HW_STATUS_ERROR;
    }
    HalStatus = HAL_DMA_Init(TxDmaHandle);

    if(HalStatus != HAL_OK)
    {
        HAL_DMA_DeInit(TxDmaHandle);
        return HW_STATUS_ERROR;
    }
    __HAL_LINKDMA(UartHandle, hdmarx, *RxDmaHandle);
    __HAL_LINKDMA(UartHandle, hdmatx, *TxDmaHandle);
    HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, DS18B20_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);

    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, DS18B20_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

    HAL_NVIC_SetPriority(USART6_IRQn, DS18B20_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(USART6_IRQn);
    return HW_STATUS_OK;
}

HW_Status_t HW_DS18B20_UartDeInit(UART_HandleTypeDef *UartHandle, DMA_HandleTypeDef *RxDmaHandle,
                                                            DMA_HandleTypeDef *TxDmaHandle)
{
	if (UartHandle->Instance == DS18B20_UART)
	{
		HAL_NVIC_DisableIRQ(DMA2_Stream2_IRQn);
		HAL_NVIC_DisableIRQ(DMA2_Stream7_IRQn);
		HAL_NVIC_DisableIRQ(USART6_IRQn);
		HAL_DMA_DeInit(RxDmaHandle);
		HAL_DMA_DeInit(TxDmaHandle);
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6 | GPIO_PIN_7);
		__HAL_RCC_USART6_CLK_DISABLE();
	}
	return HW_STATUS_OK;
}

HW_Status_t HW_DS18B20_TimerInit(TIM_HandleTypeDef *TimerHandle)
{
	RCC_ClkInitTypeDef ClkConfig = {0};
	uint32_t FlashLatency = 0U;
	uint32_t TimClock = 0U;

	__HAL_RCC_TIM7_CLK_ENABLE();
	HAL_RCC_GetClockConfig(&ClkConfig, &FlashLatency);
	if(ClkConfig.APB1CLKDivider == RCC_HCLK_DIV1)
	{
		TimClock = HAL_RCC_GetPCLK1Freq();
	}
	else
	{
		TimClock = 2U * HAL_RCC_GetPCLK1Freq();
	}

	TimerHandle->Instance = TIM7;
	TimerHandle->Init.Prescaler = (TimClock / 1000U) - 1U; // 1 kHz timer clock => 1 tick = 1 ms
	TimerHandle->Init.CounterMode = TIM_COUNTERMODE_UP;
	TimerHandle->Init.Period = 750U - 1U;
	TimerHandle->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	HAL_StatusTypeDef Status = HAL_TIM_Base_Init(TimerHandle);
	if(Status != HAL_OK)
	{
		return HW_STATUS_ERROR;
	}
	HAL_NVIC_SetPriority(TIM7_IRQn, DS18B20_IRQ_PRIORITY, 0U);
	HAL_NVIC_EnableIRQ(TIM7_IRQn);

	return HW_STATUS_OK;
}
