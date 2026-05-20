/**
 *****************************************************************************
 * @file        : eth_phy_io.c
 * @author      : embedom
 * @date        : 2026-03-15
 * @brief       : Description
 *****************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <stdio.h>
#include <stdint.h>

#include "hardware_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "eth_phy_io.h"

/********************************* DEFINES ***********************************/

/********************************* TYPEDEFS **********************************/

/*********************** PRIVATE FUNCTION PROTOTYPES *************************/

static int32_t ETH_PHY_IO_Init(void);
static int32_t ETH_PHY_IO_DeInit(void);
static int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal);
static int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal);
static int32_t ETH_PHY_IO_GetTick(void);

/***************************** PRIVATE VARIABLES *****************************/

ETH_HandleTypeDef *EthHandle;

ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __attribute__((section(".RxDecripSection")));
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __attribute__((section(".TxDecripSection")));

static lan8742_Object_t LAN8742;
static lan8742_IOCtx_t LAN8742_IOCtx = {
    ETH_PHY_IO_Init, ETH_PHY_IO_DeInit, ETH_PHY_IO_WriteReg, ETH_PHY_IO_ReadReg, ETH_PHY_IO_GetTick
};

/********************************* FUNCTIONS *********************************/

ETH_PHY_LinkState_t ETH_PHY_IO_StartInitPhy(ETH_HandleTypeDef *Handle, ETH_TxPacketConfig *TxConfig,
                                            uint8_t *MACAddr)
{
    configASSERT(Handle != NULL);
    EthHandle = Handle;
    HW_Status_t Status = HW_STATUS_ERROR;

    Status = HW_ETH_Init(Handle, TxConfig, DMATxDscrTab, DMARxDscrTab, MACAddr);
    if(Status != HW_STATUS_OK)
    {
        return ETH_PHY_LINK_STATE_ERROR;
    }

    LAN8742.IO = LAN8742_IOCtx;
    if(LAN8742_Init(&LAN8742) != LAN8742_STATUS_OK)
    {
        return ETH_PHY_LINK_STATE_ERROR;
    }
    return ETH_PHY_LINK_STATE_OK;
}

ETH_PHY_LinkState_t ETH_PHY_IO_GetLinkState(void)
{
    return LAN8742_GetLinkState(&LAN8742);
}

void ETH_PHY_IO_MapLinkState(ETH_PHY_LinkState_t PhyLinkState, uint32_t *DuplexMode,
                             uint32_t *Speed, uint8_t *LinkChanged)
{
    switch(PhyLinkState)
    {
    case ETH_PHY_LINK_STATE_100MBITS_FULLDUPLEX:
    {
        *DuplexMode = ETH_FULLDUPLEX_MODE;
        *Speed = ETH_SPEED_100M;
        *LinkChanged = 1U;
        break;
    }
    case ETH_PHY_LINK_STATE_100MBITS_HALFDUPLEX:
    {
        *DuplexMode = ETH_HALFDUPLEX_MODE;
        *Speed = ETH_SPEED_100M;
        *LinkChanged = 1U;
        break;
    }
    case ETH_PHY_LINK_STATE_10MBITS_FULLDUPLEX:
    {
        *DuplexMode = ETH_FULLDUPLEX_MODE;
        *Speed = ETH_SPEED_10M;
        *LinkChanged = 1U;
        break;
    }
    case ETH_PHY_LINK_STATE_10MBITS_HALFDUPLEX:
    {
        *DuplexMode = ETH_HALFDUPLEX_MODE;
        *Speed = ETH_SPEED_10M;
        *LinkChanged = 1U;
        break;
    }
    default:
    {
        *LinkChanged = 0U;
        break;
    }
    }
}

/***************************** PRIVATE FUNCTIONS *****************************/

/**
 * @brief  Initializes the MDIO interface and the ETH PHY.
 * @param  None
 * @retval 0 if OK, -1 if ERROR
 */
static int32_t ETH_PHY_IO_Init(void)
{
    configASSERT(EthHandle != NULL);
    /* MDIO GPIO configuration */
    HAL_ETH_MspInit(EthHandle);

    /* Configure the MDIO Clock */
    HAL_ETH_SetMDIOClockRange(EthHandle);
    return 0;
}

/**
 * @brief  De-Initializes the MDIO interface .
 * @param  None
 * @retval 0 if OK, -1 if ERROR
 */
static int32_t ETH_PHY_IO_DeInit(void)
{
    return 0;
}

/**
 * @brief  Read a PHY register through the MDIO interface.
 * @param  DevAddr: PHY port address
 * @param  RegAddr: PHY register address
 * @param  pRegVal: pointer to hold the register value
 * @retval 0 if OK -1 if Error
 */
static int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal)
{
    if(HAL_ETH_ReadPHYRegister(EthHandle, DevAddr, RegAddr, pRegVal) != HAL_OK)
    {
        return -1;
    }
    return 0;
}

/**
 * @brief  Write a value to a PHY register through the MDIO interface.
 * @param  DevAddr: PHY port address
 * @param  RegAddr: PHY register address
 * @param  RegVal: Value to be written
 * @retval 0 if OK -1 if Error
 */
static int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal)
{
    if(HAL_ETH_WritePHYRegister(EthHandle, DevAddr, RegAddr, RegVal) != HAL_OK)
    {
        return -1;
    }
    return 0;
}

/**
 * @brief  Get the time in millisecons used for internal PHY driver process.
 * @retval Time value
 */
static int32_t ETH_PHY_IO_GetTick(void)
{
    return (int32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}
