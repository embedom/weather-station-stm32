/**
 ******************************************************************************
 * @file        : http_client.hpp
 * @author      : embedom
 * @date        : 2026-04-20
 * @brief       : Blocking HTTP/1.1 client with Transport layer.
 ******************************************************************************
 */

#pragma once

/********************************* INCLUDES **********************************/

#include <array>
#include <stddef.h>
#include <cstdint>
#include "transport.hpp"

namespace Network
{

/******************************** CONSTEXPR **********************************/

constexpr size_t HTTP_MAX_HOST_LEN = 64U;
constexpr size_t HTTP_MAX_PATH_LEN = 128U;
constexpr size_t HTTP_MAX_BODY_LEN = 512U;
constexpr size_t HTTP_MAX_REQUEST_HDR_LEN = 512U;
constexpr size_t HTTP_MAX_RESPONSE_LEN = 1024U;
constexpr uint32_t HTTP_DEFAULT_TIMEOUT_MS = 5000U;

/********************************* TYPEDEFS **********************************/

enum class HttpMethod : uint8_t
{
    GET_METHOD,
    POST_METHOD,
    PUT_METHOD,
    DELETE_METHOD,
    HEAD_METHOD
};

enum class HttpStatus : uint8_t
{
    OK, /* request completed, see StatusCode */
    CONNECT_ERROR,
    DISCONNECT_ERROR,
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
    HttpClient(ITransport &Transport) : _Transport(Transport) {};
    ~HttpClient() = default;
    /**************************** Public Members *****************************/

    /**
     * @brief Initialize the client with the target endpoint.
     * @param Host Server hostname or dotted IPv4 (e.g. "192.168.1.100").
     * @param Port TCP port (e.g. 80, 8080).
     * @return OK on success, INVALID_ARGUMENT if arguments are invalid.
     */
    HttpStatus initialize(const char *Host, uint16_t Port);

    /**
     * @brief Execute a blocking GET_METHOD request.
     * @param Path Full HTTP path (e.g. "/api/weather-station/temperature").
     * @param Response Filled with the transport and HTTP response status.
     * @return OK on success, INVALID_ARGUMENT if arguments are invalid.
     */
    HttpStatus getRequest(const char *Path, HttpResponse &Response);

    /**
     * @brief Execute a blocking POST_METHOD request with body.
     * @param Path Full HTTP path.
     * @param RequestBody Null-terminated request payload.
     * @param BodyLen Length of the request body in bytes.
     * @param Response Filled with the transport and HTTP response status.
     * @return OK on success, INVALID_ARGUMENT if arguments are invalid.
     */
    HttpStatus postRequest(const char *Path, const char *RequestBody, size_t BodyLen,
                           HttpResponse &Response);

    /**
    * @brief Execute a blocking PUT_METHOD request with body.
    * @param Path Full HTTP path.
    * @param RequestBody Null-terminated request payload.
    * @param BodyLen Length of the request body in bytes.
    * @param Response Filled with the transport and HTTP response status.
    * @return OK on success, INVALID_ARGUMENT if arguments are invalid.
    */
    HttpStatus putRequest(const char *Path, const char *RequestBody, size_t BodyLen,
                          HttpResponse &Response);

    /**
     * @brief Execute a blocking DELETE_METHOD request.
     * @param Path Full HTTP path.
     * @param Response Filled with the transport and HTTP response status.
     * @return OK on success, INVALID_ARGUMENT if arguments are invalid.
     */
    HttpStatus deleteRequest(const char *Path, HttpResponse &Response);

    /**************************** Private Members ****************************/
    private:
    struct HttpRequest
    {
        HttpMethod Method;
        char Path[HTTP_MAX_PATH_LEN];
        size_t HeaderLength;
        const char *Body;
        size_t BodyLength;
    };

    enum class ReceiveDataState : uint8_t
    {
        NEED_MORE_DATA,
        DATA_COMPLETE,
        MALFORMED_RESPONSE
    };

    enum class HttpHeaderParseStatus : uint8_t
    {
        INVALID_HTTP_VERSION,
        INVALID_STATUS_CODE,
        INVALID_HEADER_END,
        INVALID_CONTENT_LENGTH,
        INVALID_BODY_LENGTH,
        INVALID_CONTENT_TYPE,
        DATA_NOT_RECEIVED,
        OK
    };

    HttpStatus executeRequest(HttpMethod Method, const char *Path, const char *Body,
                              size_t BodyLength, HttpResponse &Response);
    HttpStatus prepareRequest(HttpMethod Method, const char *Path, const char *Body,
                              size_t BodyLength, HttpRequest &Request);
    HttpStatus buildRequestHeader(HttpRequest &Request);
    HttpStatus processRequest(const HttpRequest &Request, HttpResponse &Response);
    HttpStatus sendRequest(const HttpRequest &Request);
    HttpStatus receiveResponse(HttpResponse &Response, size_t &ResponseLength);
    ReceiveDataState processReceivedData(size_t DataLength, HttpResponse &Response);
    const char *findHeaderEnd(const char *ResponseBuffer, size_t ResponseLength);
    HttpHeaderParseStatus checkResponseBodyComplete(HttpResponse &Response, size_t HeaderEndOffset,
                                                    size_t ResponseLength);
    HttpHeaderParseStatus validateHttpResponseHeaders(const char *ResponseBuffer,
                                                      size_t ResponseLength,
                                                      HttpResponse &Response);

    bool _Initialized = false;
    char _HostIp[HTTP_MAX_HOST_LEN] = {};
    uint16_t _Port = 0U;
    std::array<char, HTTP_MAX_REQUEST_HDR_LEN> _RequestHeader = {};
    std::array<char, HTTP_MAX_RESPONSE_LEN> _ResponseBuffer = {};
    ITransport &_Transport;

}; //class HttpClient

} //namespace Network
