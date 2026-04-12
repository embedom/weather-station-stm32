/**
 ******************************************************************************
 * @file        : ethernetif.h
 * @author      : embedom
 * @date        : 2026-03-01
 * @brief       : Description
 ******************************************************************************
 */

#ifndef ETHERNETIF_H
#define ETHERNETIF_H

#ifdef __cplusplus
extern "C" {
#endif

/********************************* INCLUDES **********************************/

#include "lwip/err.h"
#include "lwip/netif.h"

/**************************** FUNCTION PROTOTYPES ****************************/

err_t ethernetif_init(struct netif *netif);
u32_t sys_jiffies(void);

#ifdef __cplusplus
}
#endif

#endif /* ETHERNETIF_H */