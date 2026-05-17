/**
 ******************************************************************************
 * @file        : transport.hpp
 * @author      : embedom
 * @date        : 2026-05-16
 * @brief       : Transport layer abstraction interface to decouple client code 
 *                from the underlying transport protocol (TCP, UDP, TLS).
 ******************************************************************************
 */

#pragma once

/********************************* INCLUDES **********************************/

#include <cstdint>
#include <cstddef>

namespace Network
{

/********************************* TYPEDEFS **********************************/

enum class TransportStatus
{
    OK,
    INTERNAL_ERROR,
    INVALID_ARGUMENT,
    CONNECTION_ERROR,
    SEND_ERROR,
    RECEIVE_ERROR,
    TIMEOUT,
};

/*********************************** CLASS ***********************************/

class ITransport
{
    public:
    virtual ~ITransport() = default;

    /**
     * @brief Establish a connection to the specified host and port.
     * @param Host Null-terminated string containing the hostname or IP address to connect to.
     * @param Port Port number to connect to.
     * @param Timeout Connection timeout in milliseconds.
     * @return TransportStatus indicating the result of the operation.
     */
    virtual TransportStatus connect(const char *Host, uint16_t Port, uint32_t Timeout) = 0;

    /**
     * @brief Send data over the transport.
     * @param Data Pointer to the data buffer to send.
     * @param Length Length of the data buffer in bytes.
     * @return TransportStatus indicating the result of the operation.
     */
    virtual TransportStatus send(const uint8_t *Data, size_t Length) = 0;

    /**
     * @brief Receive data from the transport.
     * @param Buffer Pointer to the buffer where received data will be stored.
     * @param BufferSize Size of the buffer in bytes.
     * @param Received Reference to store the number of bytes received.
     * @return TransportStatus indicating the result of the operation.
     */
    virtual TransportStatus receive(uint8_t *Buffer, size_t BufferSize, size_t &Received) = 0;

    /**
     * @brief Close the transport connection.
     * @return TransportStatus indicating the result of the operation.
     */
    virtual TransportStatus close() = 0;
};

} //namespace Network
