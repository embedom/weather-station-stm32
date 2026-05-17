/**
 ******************************************************************************
 * @file        : tcp_transport.cpp
 * @author      : embedom
 * @date        : 2026-05-16
 * @brief       : TCP transport implementation using lwIP sockets.
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <stdio.h>
#include <cstdint>

#include "tcp_transport.hpp"

#include "lwip/sockets.h"
#include "lwip/inet.h"

namespace Network
{

/******************************** CONSTEXPR **********************************/

constexpr uint32_t MS_PER_SECOND = 1000U;
constexpr uint32_t US_PER_MS = 1000U;

/********************************* TYPEDEFS **********************************/

enum LwipEnumHelper
{
    LWIP_RETURN_VALUE_OK = 0,
    LWIP_SOCKET_OK = 0,
    LWIP_INVALID_SOCKET = -1,
};

/********************************** PUBLIC ***********************************/

TransportStatus TcpTransport::connect(const char *Host, uint16_t Port, uint32_t Timeout)
{
    /* DNS is disabled in this project */
    sockaddr_in ServerAddr = {};
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = lwip_htons(Port);
    if(inet_aton(Host, &ServerAddr.sin_addr) == 0U)
    {
        return TransportStatus::INVALID_ARGUMENT;
    }

    _SocketFd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if(_SocketFd < LWIP_SOCKET_OK)
    {
        return TransportStatus::INTERNAL_ERROR;
    }
    configureSocketTimeouts(Timeout);

    int ConnectResult =
        lwip_connect(_SocketFd, reinterpret_cast<sockaddr *>(&ServerAddr), sizeof(ServerAddr));
    if(ConnectResult < LWIP_RETURN_VALUE_OK)
    {
        (void)lwip_close(_SocketFd);
        _SocketFd = LWIP_INVALID_SOCKET;
        return TransportStatus::CONNECTION_ERROR;
    }
    return TransportStatus::OK;
}

TransportStatus TcpTransport::send(const uint8_t *Data, size_t Length)
{
    if((_SocketFd < LWIP_SOCKET_OK) || (Data == nullptr))
    {
        return TransportStatus::INVALID_ARGUMENT;
    }

    size_t SentTotal = 0U;
    while(SentTotal < Length)
    {
        const int Sent = lwip_send(_SocketFd, &Data[SentTotal], Length - SentTotal, 0);
        if(Sent <= 0)
        {
            return TransportStatus::SEND_ERROR;
        }
        SentTotal += static_cast<size_t>(Sent);
    }
    return TransportStatus::OK;
}

TransportStatus TcpTransport::receive(uint8_t *Buffer, size_t BufferSize, size_t &Received)
{
    if((_SocketFd < LWIP_SOCKET_OK) || (Buffer == nullptr) || (BufferSize == 0U))
    {
        return TransportStatus::INVALID_ARGUMENT;
    }

    const ssize_t ReceivedLwip = lwip_recv(_SocketFd, Buffer, BufferSize - 1U, 0);
    if(ReceivedLwip < LWIP_RETURN_VALUE_OK)
    {
        return TransportStatus::RECEIVE_ERROR;
    }
    Received = static_cast<size_t>(ReceivedLwip);
    return TransportStatus::OK;
}

TransportStatus TcpTransport::close()
{
    if(_SocketFd >= LWIP_SOCKET_OK)
    {
        (void)lwip_close(_SocketFd);
        _SocketFd = LWIP_INVALID_SOCKET;
    }
    return TransportStatus::OK;
}

/********************************** PRIVATE **********************************/

void TcpTransport::configureSocketTimeouts(uint32_t Timeout)
{
    struct timeval TimeValue;
    TimeValue.tv_sec = Timeout / MS_PER_SECOND;
    TimeValue.tv_usec =
        static_cast<suseconds_t>(Timeout % MS_PER_SECOND) * static_cast<suseconds_t>(US_PER_MS);
    (void)lwip_setsockopt(_SocketFd, SOL_SOCKET, SO_RCVTIMEO, &TimeValue, sizeof(TimeValue));
    (void)lwip_setsockopt(_SocketFd, SOL_SOCKET, SO_SNDTIMEO, &TimeValue, sizeof(TimeValue));
}

} //namespace Network
