/**
 ******************************************************************************
 * @file        : tcp_transport.hpp
 * @author      : embedom
 * @date        : 2026-05-16
 * @brief       : TCP transport implementation using lwIP sockets.
 ******************************************************************************
 */

#pragma once

/********************************* INCLUDES **********************************/

#include <cstdint>
#include "transport.hpp"

namespace Network
{

/******************************** CONSTEXPR **********************************/

/********************************* TYPEDEFS **********************************/

/*********************************** CLASS ***********************************/

class TcpTransport : public ITransport
{
    public:
    TcpTransport() = default;
    ~TcpTransport() override = default;

    virtual TransportStatus connect(const char *Host, uint16_t Port, uint32_t Timeout) override;
    virtual TransportStatus send(const uint8_t *Data, size_t Length) override;
    virtual TransportStatus receive(uint8_t *Buffer, size_t BufferSize, size_t &Received) override;
    virtual TransportStatus close() override;

    private:
    void configureSocketTimeouts(uint32_t Timeout);
    int _SocketFd = -1; // Socket file descriptor
};

} //namespace Network
