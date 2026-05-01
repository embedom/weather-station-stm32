/**
 ******************************************************************************
 * @file        : http_client.hpp
 * @author      : embedom
 * @date        : 2026-04-20
 * @brief       : Blocking HTTP/1.1 client (GET/POST) over lwIP sockets.
 ******************************************************************************
 */

#pragma once

/********************************* INCLUDES **********************************/

#include <array>
#include <stddef.h>
#include <cstdint>

namespace Network
{

/******************************** CONSTEXPR **********************************/

constexpr size_t HTTP_MAX_HOST_LEN = 64U;
constexpr size_t HTTP_MAX_PATH_LEN = 128U;
constexpr size_t HTTP_MAX_BODY_LEN = 512U;
constexpr size_t HTTP_MAX_REQUEST_HDR_LEN = 512U;
constexpr size_t HTTP_MAX_RESPONSE_LEN = 1024U;
constexpr uint32_t HTTP_SOCKET_TIMEOUT_MS = 5000U;

/********************************* TYPEDEFS **********************************/

enum class HttpMethod : uint8_t
{
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_HEAD
};

enum class HttpStatus : uint8_t
{
    OK, /* request completed, see StatusCode */
    SOCKET_ERROR,
    CONNECT_ERROR,
    SEND_ERROR,
    RECEIVE_ERROR,
    RESPONSE_TOO_LONG,
    MALFORMED_RESPONSE,
    INTERNAL_ERROR,
    INVALID_ARGUMENT,
    INVALID_PARAMETER
};

struct HttpResponse
{
    HttpStatus Status; /* transport-level status */
    int StatusCode;    /* HTTP status code (e.g. 200), valid if Status == OK */
    const char *Body;  /* pointer into internal response buffer; valid until next request */
    size_t BodyLength;
};

/*********************************** CLASS ***********************************/

class HttpClient
{
    public:
    HttpClient() = default;
    ~HttpClient() = default;
    /**************************** Public Members *****************************/

    /**
     * @brief Initialize the client with the target endpoint.
     * @param Host      Server hostname or dotted IPv4 (e.g. "192.168.1.100").
     * @param Port      TCP port (e.g. 80, 8080).
     * @return OK on success, INVALID_ARGUMENT if arguments are invalid.
     */
    HttpStatus init(const char *Host, uint16_t Port);

    /**
     * @brief Execute a blocking HTTP_METHOD_GET request.
     * @param Path      Full HTTP path (e.g. "/api/weather-station/temperature").
     * @param Response  Filled with the transport and HTTP response status.
     * @return true if the request was executed, false if arguments/client state are invalid.
     */
    bool get(const char *Path, HttpResponse &Response);

    /**
     * @brief Execute a blocking HTTP_METHOD_POST request with a JSON body.
     * @param Path      Full HTTP path.
     * @param JsonBody  Null-terminated JSON payload.
     * @param Response  Filled with the transport and HTTP response status.
     * @return true if the request was executed, false if arguments/client state are invalid.
     */
    bool post(const char *Path, const char *JsonBody, HttpResponse &Response);

    /**************************** Private Members ****************************/
    private:
    struct HttpRequest
    {
        HttpMethod Method;
        char Path[HTTP_MAX_PATH_LEN];
        const char *Body;
        size_t BodyLength;
    };

    bool executeRequest(HttpMethod Method, const char *Path, const char *Body, size_t BodyLength,
                        HttpResponse &Response);
    HttpStatus prepareRequest(HttpMethod Method, const char *Path, const char *Body,
                              size_t BodyLength, HttpRequest &Request) const;
    void processRequest(const HttpRequest &Request, HttpResponse &Response);
    HttpStatus openSocket(int &SocketDescriptor) const;
    void configureSocketTimeouts(int SocketDescriptor) const;
    bool buildRequestHeader(const HttpRequest &Request, size_t &OutLength);
    bool sendRequest(int SocketDescriptor, const HttpRequest &Request, HttpStatus &Status);
    bool sendAll(int SocketDescriptor, const char *Data, size_t Length) const;
    bool receiveResponse(int SocketDescriptor, size_t &ResponseLength, HttpStatus &Status);

    bool _Initialized = false;
    char _HostIpString[HTTP_MAX_HOST_LEN] = {};
    uint16_t _Port = 0U;
    std::array<char, HTTP_MAX_REQUEST_HDR_LEN> _RequestHeader = {};
    std::array<char, HTTP_MAX_RESPONSE_LEN> _ResponseBuffer = {};

}; //class HttpClient

} //namespace Network
