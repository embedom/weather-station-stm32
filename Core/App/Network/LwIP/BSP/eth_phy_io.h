/**
 *****************************************************************************
 * @file        : eth_phy_io.h
 * @author      : embedom
 * @date        : 2026-03-15
 * @brief       :
 *****************************************************************************
 */

#ifndef ETH_PHY_IO_H
#define ETH_PHY_IO_H

#ifdef __cplusplus
extern "C"
{
#endif

/********************************* INCLUDES **********************************/

#include <stdio.h>
#include <stdint.h>
#include "lan8742.h"

/********************************* TYPEDEFS **********************************/

typedef enum
{
    ETH_PHY_LINK_STATE_READ_ERROR = LAN8742_STATUS_READ_ERROR,
    ETH_PHY_LINK_STATE_WRITE_ERROR = LAN8742_STATUS_WRITE_ERROR,
    ETH_PHY_LINK_STATE_ADDRESS_ERROR = LAN8742_STATUS_ADDRESS_ERROR,
    ETH_PHY_LINK_STATE_RESET_TIMEOUT = LAN8742_STATUS_RESET_TIMEOUT,
    ETH_PHY_LINK_STATE_ERROR = LAN8742_STATUS_ERROR,
    ETH_PHY_LINK_STATE_OK = LAN8742_STATUS_OK,
    ETH_PHY_LINK_STATE_LINK_DOWN = LAN8742_STATUS_LINK_DOWN,
    ETH_PHY_LINK_STATE_100MBITS_FULLDUPLEX = LAN8742_STATUS_100MBITS_FULLDUPLEX,
    ETH_PHY_LINK_STATE_100MBITS_HALFDUPLEX = LAN8742_STATUS_100MBITS_HALFDUPLEX,
    ETH_PHY_LINK_STATE_10MBITS_FULLDUPLEX = LAN8742_STATUS_10MBITS_FULLDUPLEX,
    ETH_PHY_LINK_STATE_10MBITS_HALFDUPLEX = LAN8742_STATUS_10MBITS_HALFDUPLEX,
    ETH_PHY_LINK_STATE_AUTONEGO_NOTDONE = LAN8742_STATUS_AUTONEGO_NOTDONE
} ETH_PHY_LinkState_t;

/**************************** FUNCTION PROTOTYPES ****************************/

ETH_PHY_LinkState_t ETH_PHY_IO_StartInitPhy(ETH_HandleTypeDef *Handle, ETH_TxPacketConfig *TxConfig,
                                            uint8_t *MACAddr);

ETH_PHY_LinkState_t ETH_PHY_IO_GetLinkState(void);

void ETH_PHY_IO_MapLinkState(ETH_PHY_LinkState_t PHYLinkState, uint32_t *DuplexMode,
                             uint32_t *Speed, uint8_t *LinkChanged);

#ifdef __cplusplus
}
#endif

#endif /* ETH_PHY_IO_H */