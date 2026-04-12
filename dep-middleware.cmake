############################# FreeRTOS config ################################
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-Kernel/include
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-Kernel/portable/GCC/ARM_CM7/r0p1
)

set(FREERTOS_SOURCES
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-Kernel/event_groups.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-Kernel/list.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-Kernel/queue.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-Kernel/stream_buffer.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-Kernel/tasks.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-Kernel/timers.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-Kernel/portable/MemMang/heap_4.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-Kernel/portable/GCC/ARM_CM7/r0p1/port.c
    # ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-Kernel/portable/Common/mpu_wrappers.c
    # ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-Kernel/portable/Common/mpu_wrappers_v2.c
)

############################## LwIP cmake config ##############################
set(LWIP_DIR_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/lwip)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/Middlewares/lwip/src/include
)

# The minimum set of files needed for lwIP.
set(lwipcore_SRCS
    ${LWIP_DIR_PATH}/src/core/init.c
    ${LWIP_DIR_PATH}/src/core/def.c
    ${LWIP_DIR_PATH}/src/core/dns.c
    ${LWIP_DIR_PATH}/src/core/inet_chksum.c
    ${LWIP_DIR_PATH}/src/core/ip.c
    ${LWIP_DIR_PATH}/src/core/mem.c
    ${LWIP_DIR_PATH}/src/core/memp.c
    ${LWIP_DIR_PATH}/src/core/netif.c
    ${LWIP_DIR_PATH}/src/core/pbuf.c
    ${LWIP_DIR_PATH}/src/core/raw.c
    ${LWIP_DIR_PATH}/src/core/stats.c
    ${LWIP_DIR_PATH}/src/core/sys.c
    ${LWIP_DIR_PATH}/src/core/altcp.c
    ${LWIP_DIR_PATH}/src/core/altcp_alloc.c
    ${LWIP_DIR_PATH}/src/core/altcp_tcp.c
    ${LWIP_DIR_PATH}/src/core/tcp.c
    ${LWIP_DIR_PATH}/src/core/tcp_in.c
    ${LWIP_DIR_PATH}/src/core/tcp_out.c
    ${LWIP_DIR_PATH}/src/core/timeouts.c
    ${LWIP_DIR_PATH}/src/core/udp.c
)
set(lwipcore4_SRCS
    ${LWIP_DIR_PATH}/src/core/ipv4/acd.c
    ${LWIP_DIR_PATH}/src/core/ipv4/autoip.c
    ${LWIP_DIR_PATH}/src/core/ipv4/dhcp.c
    ${LWIP_DIR_PATH}/src/core/ipv4/etharp.c
    ${LWIP_DIR_PATH}/src/core/ipv4/icmp.c
    ${LWIP_DIR_PATH}/src/core/ipv4/igmp.c
    ${LWIP_DIR_PATH}/src/core/ipv4/ip4_frag.c
    ${LWIP_DIR_PATH}/src/core/ipv4/ip4.c
    ${LWIP_DIR_PATH}/src/core/ipv4/ip4_addr.c
)

# APIFILES: The files which implement the sequential and socket APIs.
set(lwipapi_SRCS
    ${LWIP_DIR_PATH}/src/api/api_lib.c
    ${LWIP_DIR_PATH}/src/api/api_msg.c
    ${LWIP_DIR_PATH}/src/api/err.c
    ${LWIP_DIR_PATH}/src/api/if_api.c
    ${LWIP_DIR_PATH}/src/api/netbuf.c
    ${LWIP_DIR_PATH}/src/api/netdb.c
    ${LWIP_DIR_PATH}/src/api/netifapi.c
    ${LWIP_DIR_PATH}/src/api/sockets.c
    ${LWIP_DIR_PATH}/src/api/tcpip.c
)

# Files implementing various generic network interface functions
set(lwipnetif_SRCS
    ${LWIP_DIR_PATH}/src/netif/ethernet.c
    # ${LWIP_DIR_PATH}/src/netif/bridgeif.c
    # ${LWIP_DIR_PATH}/src/netif/bridgeif_fdb.c
)

set(LWIP_SRCS
    ${lwipcore_SRCS}
    ${lwipcore4_SRCS}
    ${lwipapi_SRCS}
    ${lwipnetif_SRCS}
)

# Suppress compiler warnings for third-party middleware sources
set_source_files_properties(${FREERTOS_SOURCES} ${LWIP_SRCS} PROPERTIES COMPILE_OPTIONS "-w")
