/**
 ******************************************************************************
 * @file        : ethernetif.c
 * @author      : embedom
 * @date        : 2026-03-15
 * @brief       : Link layer for ethernet phy interface
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "misc_compiler.h"
#include "hardware_config.h"
#include "eth_phy_io.h"

#include "lwip/opt.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "netif/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/sys.h"
#include "lwip/snmp.h"
#include "lwip/tcpip.h"

#include "FreeRTOS.h"
#include "semphr.h"

/********************************* DEFINES ***********************************/

#define LINK_NETIF_SPEED_IN_BPS       100000000U /* 100Mbps */
#define ETH_RX_BUFFER_CNT             4U

/* The time to block waiting for input. */
#define TIME_WAITING_FOR_INPUT (portMAX_DELAY)

/* Stack size of the interface thread */
#define INTERFACE_THREAD_STACK_SIZE   (350U)

/* Network interface name */
#define IFNAME0 's'
#define IFNAME1 't'

/* ETH Setting  */
#define ETH_DMA_TRANSMIT_TIMEOUT      (20U)
#define ETH_TX_BUFFER_MAX             ((ETH_TX_DESC_CNT) * 2U)

/********************************* TYPEDEFS **********************************/

/* Data Type Definitions */
typedef enum
{
    RX_ALLOC_OK = 0x00,
    RX_ALLOC_ERROR = 0x01
} RxAllocStatusTypeDef;

typedef struct
{
    struct pbuf_custom pbuf_custom;
    uint8_t buff[(ETH_RX_BUF_SIZE + 31) & ~31] __ALIGNED(32);
} RxBuff_t;

/*********************** PRIVATE FUNCTION PROTOTYPES *************************/

static void low_level_init(struct netif *NetIf);
static struct pbuf *low_level_input(struct netif *NetIf);
static err_t low_level_output(struct netif *NetIf, struct pbuf *p);
static void pbuf_free_custom(struct pbuf *Buffer);

static void ethernetif_input_thread(void *Argument);
static void ethernet_link_thread(void *Argument);

/***************************** PRIVATE VARIABLES *****************************/

/* Global Ethernet handle */
ETH_HandleTypeDef EthernetHandle;
ETH_TxPacketConfig TxConfig;

/* Variable Definitions */
static uint8_t RxAllocStatus;

/* Memory Pool Declaration */
LWIP_MEMPOOL_DECLARE(RX_POOL, ETH_RX_BUFFER_CNT, sizeof(RxBuff_t), "Zero-copy RX PBUF pool")

/* FreeRTOS config */
static StaticSemaphore_t RxPktSemaphoreStorage;
static StaticSemaphore_t TxPktSemaphoreStorage;
SemaphoreHandle_t RxPktSemaphore = NULL; /* Semaphore to signal incoming packets */
SemaphoreHandle_t TxPktSemaphore = NULL; /* Semaphore to signal transmit packet complete */

/********************************* FUNCTIONS *********************************/

#if !LWIP_ARP
/**
 * This function has to be completed by user in case of ARP OFF.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if ...
 */
static err_t low_level_output_arp_off(struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr)
{
    err_t errval;
    errval = ERR_OK;
    return errval;
}
#endif /* LWIP_ARP */

/**
 * @brief Called to set up the network interface. It calls low_level_init()
 * to do the actual setup of the hardware.
 * Should be passed as a parameter to netif_add().
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t ethernetif_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_NETIF_SPEED_IN_BPS);

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;

#if LWIP_IPV4
#if LWIP_ARP || LWIP_ETHERNET
    netif->output = etharp_output;
#else
    netif->output = low_level_output_arp_off;
#endif /* LWIP_ARP || LWIP_ETHERNET */
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif

    netif->linkoutput = low_level_output;

    /* initialize the hardware */
    low_level_init(netif);

    return ERR_OK;
}

/***************************** PRIVATE FUNCTIONS *****************************/

/**
 * @brief Initializaiotn of the hardware. This should be called from ethernetif_init().
 * @param NetIf: Already initialized lwip network interface structure for this ethernetif
 * @retval None
 */
static void low_level_init(struct netif *NetIf)
{
    int32_t EthInitStatus = 0U;
    /* Initialize the RX POOL */
    LWIP_MEMPOOL_INIT(RX_POOL);
    RxAllocStatus = RX_ALLOC_OK;

    /* create a binary semaphore used for informing ethernetif of frame reception/transmission */
    RxPktSemaphore = xSemaphoreCreateBinaryStatic(&RxPktSemaphoreStorage);
    TxPktSemaphore = xSemaphoreCreateBinaryStatic(&TxPktSemaphoreStorage);
    if((RxPktSemaphore == NULL) || (TxPktSemaphore == NULL))
    {
        Error_Handler();
    }
    vQueueAddToRegistry(RxPktSemaphore, "RxPktSem");
    vQueueAddToRegistry(TxPktSemaphore, "TxPktSem");

    /* maximum transfer unit */
    NetIf->mtu = ETH_MAX_PAYLOAD;
    NetIf->hwaddr_len = ETH_HWADDR_LEN;

    uint8_t MacAddr[ETH_HWADDR_LEN];
    /* Initialize the ETH PHY and monitor link state */
    EthInitStatus = ETH_PHY_IO_StartInitPhy(&EthernetHandle, &TxConfig, MacAddr);

    if(EthInitStatus == ETH_PHY_LINK_STATE_OK)
    {
        /* set MAC hardware address */
        memcpy(NetIf->hwaddr, MacAddr, ETH_HWADDR_LEN);

/* Accept broadcast address and ARP traffic */
#if LWIP_ARP
        NetIf->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
#else
        NetIf->flags |= NETIF_FLAG_BROADCAST;
#endif /* LWIP_ARP */

        BaseType_t xReturned;
        /* create the task that handles the ETH_MAC */
        xReturned = xTaskCreate(ethernetif_input_thread,
                                "EthIfInput",
                                INTERFACE_THREAD_STACK_SIZE,
                                NetIf,
                                (tskIDLE_PRIORITY + 4),
                                NULL);
        if(xReturned != pdPASS)
        {
            Error_Handler();
        }
        /* monitor link changes in the background (detects link up when starting with the cable disconnected) */
        xReturned = xTaskCreate(ethernet_link_thread,
                                "EthIfLink",
                                INTERFACE_THREAD_STACK_SIZE,
                                NetIf,
                                (tskIDLE_PRIORITY + 3),
                                NULL);
        if(xReturned != pdPASS)
        {
            Error_Handler();
        }
        netif_set_link_down(NetIf);
        netif_set_down(NetIf);
    }
    else
    {
        Error_Handler();
    }
}

/**
 * @brief This function do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf might be chained.
 * @param NetIf the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
static err_t low_level_output(struct netif *NetIf, struct pbuf *p)
{
    (void)NetIf;
    uint32_t i = 0U;
    struct pbuf *q = NULL;
    err_t ErrVal = ERR_OK;
    ETH_BufferTypeDef TxBuffer[ETH_TX_DESC_CNT] = { 0 };

    memset(TxBuffer, 0, ETH_TX_DESC_CNT * sizeof(ETH_BufferTypeDef));

    for(q = p; q != NULL; q = q->next, i++)
    {
        if(i >= ETH_TX_DESC_CNT)
        {
            return ERR_IF;
        }
        TxBuffer[i].buffer = q->payload;
        TxBuffer[i].len = q->len;

        if(i > 0)
        {
            TxBuffer[i - 1].next = &TxBuffer[i];
        }
        if(q->next == NULL)
        {
            TxBuffer[i].next = NULL;
        }
    }

    TxConfig.Length = p->tot_len;
    TxConfig.TxBuffer = TxBuffer;
    TxConfig.pData = p;
    pbuf_ref(p);

    if(HAL_ETH_Transmit_IT(&EthernetHandle, &TxConfig) == HAL_OK)
    {
        while(xSemaphoreTake(TxPktSemaphore, ETH_DMA_TRANSMIT_TIMEOUT) != pdTRUE);
        HAL_ETH_ReleaseTxPacket(&EthernetHandle);
    }
    else
    {
        pbuf_free(p);
    }
    return ErrVal;
}

/**
 * @brief Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 * @param NetIf the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
  */
static struct pbuf *low_level_input(struct netif *NetIf)
{
    (void)NetIf;
    struct pbuf *RxBuffer = NULL;

    if(RxAllocStatus == RX_ALLOC_OK)
    {
        HAL_ETH_ReadData(&EthernetHandle, (void **)&RxBuffer);
    }
    return RxBuffer;
}

/**
  * @brief  Custom Rx pbuf free callback
  * @param  Buffer: pbuf to be freed
  * @retval None
  */
static void pbuf_free_custom(struct pbuf *Buffer)
{
    struct pbuf_custom *CustomBuf = (struct pbuf_custom *)Buffer;
    LWIP_MEMPOOL_FREE(RX_POOL, CustomBuf);

    /* If the Rx Buffer Pool was exhausted, signal the ethernetif_input task to
    * call HAL_ETH_GetRxDataBuffer to rebuild the Rx descriptors. */
    if(RxAllocStatus == RX_ALLOC_ERROR)
    {
        RxAllocStatus = RX_ALLOC_OK;
        xSemaphoreGive(RxPktSemaphore);
    }
}

/********************************** THREADS **********************************/

/**
  * @brief  Check the ETH link state then update ETH driver and netif link accordingly.
  * @param Argument the lwip network interface structure for this ethernetif
  * @retval None
  */
static void ethernet_link_thread(void *Argument)
{
    struct netif *NetIf = (struct netif *)Argument;
    ETH_PHY_LinkState_t PhyLinkState = 0;
    uint8_t LinkChanged = 0U;
    uint32_t Speed = 0U, DuplexMode = 0U;

    for(;;)
    {
        LinkChanged = 0U;
        PhyLinkState = ETH_PHY_IO_GetLinkState();

        if(netif_is_link_up(NetIf) && (PhyLinkState <= ETH_PHY_LINK_STATE_LINK_DOWN))
        {
            HAL_ETH_Stop_IT(&EthernetHandle);
            NetIf->link_speed = 0U;
            netif_set_down(NetIf);
            netif_set_link_down(NetIf);
        }
        else if(!netif_is_link_up(NetIf) && (PhyLinkState > ETH_PHY_LINK_STATE_LINK_DOWN))
        {
            ETH_PHY_IO_MapLinkState(PhyLinkState, &DuplexMode, &Speed, &LinkChanged);
            if(LinkChanged)
            {
                if(HW_ETH_update_mac_config(&EthernetHandle, DuplexMode, Speed) == HW_STATUS_OK)
                {
                    NetIf->link_speed = (Speed == ETH_SPEED_100M ? LINK_NETIF_SPEED_IN_BPS
                                                                 : (LINK_NETIF_SPEED_IN_BPS / 10U));
                    netif_set_up(NetIf);
                    netif_set_link_up(NetIf);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 * @param Argument the lwip network interface structure for this ethernetif
 * @retval None
 */
static void ethernetif_input_thread(void *Argument)
{
    struct pbuf *RxBuffer = NULL;
    struct netif *NetIf = (struct netif *)Argument;

    for(;;)
    {
        if(xSemaphoreTake(RxPktSemaphore, TIME_WAITING_FOR_INPUT) == pdTRUE)
        {
            do
            {
                RxBuffer = low_level_input(NetIf);
                if(RxBuffer != NULL)
                {
                    if(NetIf->input(RxBuffer, NetIf) != ERR_OK)
                    {
                        pbuf_free(RxBuffer);
                    }
                }
            } while(RxBuffer != NULL);
        }
    }
}

/******************************** INTERRUPTS *********************************/

void ETH_IRQHandler(void)
{
    HAL_ETH_IRQHandler(&EthernetHandle);
}

void ETH_WKUP_IRQHandler(void)
{
    HAL_ETH_IRQHandler(&EthernetHandle);
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
    struct pbuf **ppStart = (struct pbuf **)pStart;
    struct pbuf **ppEnd = (struct pbuf **)pEnd;
    struct pbuf *p = NULL;

    /* Get the struct pbuf from the buff address. */
    p = (struct pbuf *)(buff - offsetof(RxBuff_t, buff));
    p->next = NULL;
    p->tot_len = 0;
    p->len = Length;

    /* Chain the buffer. */
    if(!*ppStart)
    {
        *ppStart = p; /* The first buffer of the packet. */
    }
    else
    {
        (*ppEnd)->next = p; /* Chain the buffer to the end of the packet. */
    }
    *ppEnd = p;

    /* Update the total length of all the buffers of the chain. Each pbuf in the chain should 
        have its tot_len set to its own length, plus the length of all the following pbufs in the chain. */
    for(p = *ppStart; p != NULL; p = p->next)
    {
        p->tot_len += Length;
    }
}

void HAL_ETH_TxFreeCallback(uint32_t *buff)
{
    pbuf_free((struct pbuf *)buff);
}

void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
    struct pbuf_custom *p = LWIP_MEMPOOL_ALLOC(RX_POOL);
    if(p)
    {
        /* Get the buff from the struct pbuf address. */
        *buff = (uint8_t *)p + offsetof(RxBuff_t, buff);
        p->custom_free_function = pbuf_free_custom;
        /* Initialize the struct pbuf.
        * This must be performed whenever a buffer's allocated because it may be
        * changed by lwIP or the app, e.g., pbuf_free decrements ref. */
        pbuf_alloced_custom(PBUF_RAW, 0, PBUF_REF, p, *buff, ETH_RX_BUF_SIZE);
    }
    else
    {
        RxAllocStatus = RX_ALLOC_ERROR;
        *buff = NULL;
    }
}

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *handlerEth)
{
    (void)handlerEth;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(RxPktSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *HandlerEth)
{
    (void)HandlerEth;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(TxPktSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *HandlerEth)
{
    if((HAL_ETH_GetDMAError(HandlerEth) & ETH_DMASR_RBUS) == ETH_DMASR_RBUS)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(RxPktSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
