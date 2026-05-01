/**
 ******************************************************************************
 * @file        : http_client.cpp
 * @author      : embedom
 * @date        : 2026-04-20
 * @brief       : Blocking HTTP/1.1 client (GET/POST) over lwIP sockets.
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <stdio.h>
#include <string.h>

#include "SEGGER_RTT.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "app_config.hpp"
#include "http_client.hpp"
#include "http_response_parser.hpp"

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

class SocketGuard
{
    public:
    explicit SocketGuard(int Socket) : _Socket(Socket) {}
    ~SocketGuard()
    {
        if(_Socket >= 0)
        {
            (void)lwip_close(_Socket);
        }
    }

    SocketGuard(const SocketGuard &) = delete;
    SocketGuard &operator=(const SocketGuard &) = delete;

    private:
    int _Socket;
};

/****************************** STATIC HELPER ********************************/

static constexpr const char *methodToString(HttpMethod Method)
{
    switch(Method)
    {
    case HttpMethod::HTTP_METHOD_GET:
        return "GET";
    case HttpMethod::HTTP_METHOD_POST:
        return "POST";
    case HttpMethod::HTTP_METHOD_PUT:
        return "PUT";
    case HttpMethod::HTTP_METHOD_DELETE:
        return "DELETE";
    case HttpMethod::HTTP_METHOD_HEAD:
        return "HEAD";
    }
    return "UNKNOWN";
}

/********************************** PUBLIC ***********************************/

HttpStatus HttpClient::init(const char *Host, uint16_t Port)
{
    if(_Initialized || (Host == nullptr) || (Port == 0U))
    {
        return HttpStatus::INVALID_ARGUMENT;
    }

    size_t HostLen = strlen(Host);
    if(HostLen >= HTTP_MAX_HOST_LEN)
    {
        return HttpStatus::INVALID_ARGUMENT;
    }

    memcpy(_HostIpString, Host, HostLen + 1U); /* include null terminator */
    _Port = Port;
    _Initialized = true;
    return HttpStatus::OK;
}

bool HttpClient::get(const char *ApiPath, HttpResponse &Response)
{
    return executeRequest(HttpMethod::HTTP_METHOD_GET, ApiPath, nullptr, 0U, Response);
}

bool HttpClient::post(const char *ApiPath, const char *JsonBody, HttpResponse &Response)
{
    const size_t BodyLen = (JsonBody != nullptr) ? strlen(JsonBody) : 0U;
    return executeRequest(HttpMethod::HTTP_METHOD_POST, ApiPath, JsonBody, BodyLen, Response);
}

/********************************** PRIVATE **********************************/

bool HttpClient::executeRequest(HttpMethod Method, const char *ApiPath, const char *Body,
                                size_t BodyLength, HttpResponse &Response)
{
    HttpRequest Request = {};
    if(prepareRequest(Method, ApiPath, Body, BodyLength, Request) != HttpStatus::OK)
    {
        Response = {};
        Response.Status = HttpStatus::INTERNAL_ERROR;
        return false;
    }

    processRequest(Request, Response);
    return true;
}

HttpStatus HttpClient::prepareRequest(HttpMethod Method, const char *ApiPath, const char *Body,
                                      size_t BodyLength, HttpRequest &Request) const
{
    if(!_Initialized || (ApiPath == nullptr))
    {
        return HttpStatus::INVALID_ARGUMENT;
    }
    if((BodyLength >= HTTP_MAX_BODY_LEN) || ((BodyLength > 0U) && (Body == nullptr)))
    {
        return HttpStatus::INVALID_PARAMETER;
    }

    Request = {};
    Request.Method = Method;
    Request.Body = Body;
    Request.BodyLength = BodyLength;

    const size_t PathLen = strlen(ApiPath);
    if((PathLen == 0U) || (ApiPath[0] != '/') || (PathLen >= sizeof(Request.Path)))
    {
        return HttpStatus::INVALID_PARAMETER;
    }

    memcpy(Request.Path, ApiPath, PathLen + 1U); /* include null terminator */
    return HttpStatus::OK;
}

void HttpClient::processRequest(const HttpRequest &Request, HttpResponse &Response)
{
    Response = {};
    Response.Status = HttpStatus::OK;

    int SocketDescriptor = LWIP_INVALID_SOCKET;
    HttpStatus Status = openSocket(SocketDescriptor);
    if(Status != HttpStatus::OK)
    {
        Response.Status = Status;
        return;
    }
    SocketGuard SockGuard(SocketDescriptor);

    if(!sendRequest(SocketDescriptor, Request, Status))
    {
        Response.Status = Status;
        return;
    }

    size_t ResponseLength = 0U;
    if(!receiveResponse(SocketDescriptor, ResponseLength, Status))
    {
        Response.Status = Status;
        return;
    }

    if(parseHttpResponse(_ResponseBuffer.data(), ResponseLength, Response) !=
       HttpHeaderParseStatus::OK)
    {
        Response.Status = HttpStatus::MALFORMED_RESPONSE;
        return;
    }
}

HttpStatus HttpClient::openSocket(int &SocketDescriptor) const
{
    /* DNS is disabled in this project */
    struct sockaddr_in ServerAddr = {};
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = lwip_htons(_Port);
    if(inet_aton(_HostIpString, &ServerAddr.sin_addr) == LWIP_RETURN_VALUE_OK)
    {
        return HttpStatus::INVALID_PARAMETER;
    }

    SocketDescriptor = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if(SocketDescriptor < LWIP_SOCKET_OK)
    {
        return HttpStatus::SOCKET_ERROR;
    }
    configureSocketTimeouts(SocketDescriptor);

    int ConnectResult = lwip_connect(
        SocketDescriptor, reinterpret_cast<sockaddr *>(&ServerAddr), sizeof(ServerAddr));
    if(ConnectResult < LWIP_RETURN_VALUE_OK)
    {
        (void)lwip_close(SocketDescriptor);
        SocketDescriptor = LWIP_INVALID_SOCKET;
        return HttpStatus::CONNECT_ERROR;
    }
    return HttpStatus::OK;
}

void HttpClient::configureSocketTimeouts(int SocketDescriptor) const
{
    struct timeval TimeValue;
    TimeValue.tv_sec = HTTP_SOCKET_TIMEOUT_MS / MS_PER_SECOND;
    TimeValue.tv_usec = (HTTP_SOCKET_TIMEOUT_MS % MS_PER_SECOND) * US_PER_MS;
    (void)lwip_setsockopt(SocketDescriptor, SOL_SOCKET, SO_RCVTIMEO, &TimeValue, sizeof(TimeValue));
    (void)lwip_setsockopt(SocketDescriptor, SOL_SOCKET, SO_SNDTIMEO, &TimeValue, sizeof(TimeValue));
}

bool HttpClient::buildRequestHeader(const HttpRequest &Request, size_t &OutLength)
{
    int Written = 0;
    if(Request.Method == HttpMethod::HTTP_METHOD_POST)
    {
        Written = snprintf(_RequestHeader.data(),
                           _RequestHeader.size(),
                           "%s %s HTTP/1.1\r\n"
                           "Host: %s:%u\r\n"
                           "User-Agent: %s\r\n"
                           "Accept: application/json\r\n"
                           "Connection: close\r\n"
                           "Content-Type: application/json\r\n"
                           "Content-Length: %u\r\n"
                           "\r\n",
                           methodToString(HttpMethod::HTTP_METHOD_POST),
                           Request.Path,
                           _HostIpString,
                           _Port,
                           USER_AGENT_STRING,
                           static_cast<unsigned>(Request.BodyLength));
    }
    else
    {
        Written = snprintf(_RequestHeader.data(),
                           _RequestHeader.size(),
                           "%s %s HTTP/1.1\r\n"
                           "Host: %s:%u\r\n"
                           "User-Agent: %s\r\n"
                           "Accept: application/json\r\n"
                           "Connection: close\r\n"
                           "\r\n",
                           methodToString(HttpMethod::HTTP_METHOD_GET),
                           Request.Path,
                           _HostIpString,
                           _Port,
                           USER_AGENT_STRING);
    }

    if((Written <= 0) || (static_cast<size_t>(Written) >= _RequestHeader.size()))
    {
        OutLength = 0U;
        return false;
    }
    OutLength = static_cast<size_t>(Written);
    return true;
}

bool HttpClient::sendRequest(int SocketDescriptor, const HttpRequest &Request, HttpStatus &Status)
{
    size_t HeaderLen = 0U;
    if(!buildRequestHeader(Request, HeaderLen))
    {
        Status = HttpStatus::INTERNAL_ERROR;
        return false;
    }

    if(!sendAll(SocketDescriptor, _RequestHeader.data(), HeaderLen))
    {
        Status = HttpStatus::SEND_ERROR;
        return false;
    }

    if((Request.Method == HttpMethod::HTTP_METHOD_POST) && (Request.BodyLength > 0U))
    {
        if(!sendAll(SocketDescriptor, Request.Body, Request.BodyLength))
        {
            Status = HttpStatus::SEND_ERROR;
            return false;
        }
    }

    Status = HttpStatus::OK;
    return true;
}

bool HttpClient::sendAll(int SocketDescriptor, const char *Data, size_t Length) const
{
    size_t SentTotal = 0U;
    while(SentTotal < Length)
    {
        const int Sent = lwip_send(SocketDescriptor, &Data[SentTotal], Length - SentTotal, 0);
        if(Sent <= 0)
        {
            return false;
        }
        SentTotal += static_cast<size_t>(Sent);
    }
    return true;
}

bool HttpClient::receiveResponse(int SocketDescriptor, size_t &ResponseLength, HttpStatus &Status)
{
    ResponseLength = 0U;
    for(;;)
    {
        if(ResponseLength >= (sizeof(_ResponseBuffer) - 1U))
        {
            Status = HttpStatus::RESPONSE_TOO_LONG;
            return false;
        }

        const int Received = lwip_recv(SocketDescriptor,
                                       _ResponseBuffer.data() + ResponseLength,
                                       _ResponseBuffer.size() - 1U - ResponseLength,
                                       0);
        if(Received < 0)
        {
            Status = HttpStatus::RECEIVE_ERROR;
            return false;
        }
        if(Received == 0) /* Connection closed by peer - we send 'Connection: close' */
        {
            break;
        }
        ResponseLength += static_cast<size_t>(Received);
    }

    _ResponseBuffer[ResponseLength] = '\0';
    Status = HttpStatus::OK;
    return true;
}

} //namespace Network
