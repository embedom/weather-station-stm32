/**
 ******************************************************************************
 * @file        : http_client.cpp
 * @author      : embedom
 * @date        : 2026-04-20
 * @brief       : Blocking HTTP/1.1 client with Transport layer.
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <stdio.h>
#include <string.h>

#include "SEGGER_RTT.h"
#include "app_config.hpp"
#include "http_client.hpp"

namespace Network
{

/******************************** CONSTEXPR **********************************/

static constexpr char HTTP_HEADER_FORMAT[] = "%s %s HTTP/1.1\r\n"
                                             "Host: %s:%u\r\n"
                                             "User-Agent: %s\r\n"
                                             "Accept: application/json\r\n"
                                             "Connection: close\r\n"
                                             "\r\n";

static constexpr char HTTP_HEADER_WITH_BODY_FORMAT[] = "%s %s HTTP/1.1\r\n"
                                                       "Host: %s:%u\r\n"
                                                       "User-Agent: %s\r\n"
                                                       "Accept: application/json\r\n"
                                                       "Connection: close\r\n"
                                                       "Content-Type: application/json\r\n"
                                                       "Content-Length: %u\r\n"
                                                       "\r\n";

/****************************** STATIC HELPER ********************************/

static constexpr const char *methodToString(HttpMethod Method)
{
    switch(Method)
    {
    case HttpMethod::GET_METHOD:
        return "GET";
    case HttpMethod::POST_METHOD:
        return "POST";
    case HttpMethod::PUT_METHOD:
        return "PUT";
    case HttpMethod::DELETE_METHOD:
        return "DELETE";
    case HttpMethod::HEAD_METHOD:
        return "HEAD";
    }
    return "UNKNOWN";
}

/********************************** PUBLIC ***********************************/

HttpStatus HttpClient::initialize(const char *Host, uint16_t Port)
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

    memcpy(_HostIp, Host, HostLen + 1U); /* include null terminator */
    _Port = Port;
    _Initialized = true;
    return HttpStatus::OK;
}

HttpStatus HttpClient::getRequest(const char *ApiPath, HttpResponse &Response)
{
    return executeRequest(HttpMethod::GET_METHOD, ApiPath, nullptr, 0U, Response);
}

HttpStatus HttpClient::postRequest(const char *ApiPath, const char *RequestBody, size_t BodyLen,
                                   HttpResponse &Response)
{
    return executeRequest(HttpMethod::POST_METHOD, ApiPath, RequestBody, BodyLen, Response);
}

HttpStatus HttpClient::putRequest(const char *ApiPath, const char *RequestBody, size_t BodyLen,
                                  HttpResponse &Response)
{
    return executeRequest(HttpMethod::PUT_METHOD, ApiPath, RequestBody, BodyLen, Response);
}

HttpStatus HttpClient::deleteRequest(const char *ApiPath, HttpResponse &Response)
{
    return executeRequest(HttpMethod::DELETE_METHOD, ApiPath, nullptr, 0U, Response);
}

/********************************** PRIVATE **********************************/

HttpStatus HttpClient::executeRequest(HttpMethod Method, const char *ApiPath, const char *Body,
                                      size_t BodyLength, HttpResponse &Response)
{
    HttpStatus Status = HttpStatus::INTERNAL_ERROR;
    if(_Initialized && (ApiPath != nullptr))
    {
        HttpRequest Request = {};
        Status = prepareRequest(Method, ApiPath, Body, BodyLength, Request);
        if(Status == HttpStatus::OK)
        {
            Status = processRequest(Request, Response);
        }
        Response.Status = Status;
    }
    return Status;
}

HttpStatus HttpClient::prepareRequest(HttpMethod Method, const char *ApiPath, const char *Body,
                                      size_t BodyLength, HttpRequest &Request)
{
    HttpStatus Status = HttpStatus::OK;
    if((BodyLength >= HTTP_MAX_BODY_LEN) || ((BodyLength > 0U) && (Body == nullptr)))
    {
        return HttpStatus::INVALID_PARAMETER;
    }

    Request.Method = Method;
    Request.Body = Body;
    Request.BodyLength = BodyLength;

    const size_t PathLen = strlen(ApiPath);
    if((PathLen == 0U) || (ApiPath[0] != '/') || (PathLen >= sizeof(Request.Path)))
    {
        return HttpStatus::INVALID_PARAMETER;
    }
    memcpy(Request.Path, ApiPath, PathLen + 1U); /* include null terminator */

    Status = buildRequestHeader(Request);
    return Status;
}

HttpStatus HttpClient::processRequest(const HttpRequest &Request, HttpResponse &Response)
{
    Response = {};
    Response.Status = HttpStatus::OK;
    HttpStatus Status = HttpStatus::OK;
    size_t ResponseLength = 0U;

    TransportStatus ConStatus = _Transport.connect(_HostIp, _Port, HTTP_DEFAULT_TIMEOUT_MS);
    if(ConStatus != TransportStatus::OK)
    {
        return HttpStatus::CONNECT_ERROR;
    }

    Status = sendRequest(Request);
    if(Status == HttpStatus::OK)
    {
        Status = receiveResponse(Response, ResponseLength);
    }

    ConStatus = _Transport.close();
    if(ConStatus != TransportStatus::OK && Status == HttpStatus::OK)
    {
        Status = HttpStatus::DISCONNECT_ERROR;
    }

    if(Status != HttpStatus::OK)
    {
        return Status;
    }

    if(validateHttpResponseHeaders(_ResponseBuffer.data(), ResponseLength, Response) !=
       HttpHeaderParseStatus::OK)
    {
        return HttpStatus::MALFORMED_RESPONSE;
    }
    return Status;
}

HttpStatus HttpClient::sendRequest(const HttpRequest &Request)
{
    HttpStatus Status = HttpStatus::OK;

    TransportStatus SendStatus = _Transport.send(
        reinterpret_cast<const uint8_t *>(_RequestHeader.data()), Request.HeaderLength);
    if(SendStatus == TransportStatus::OK)
    {
        if(Request.BodyLength > 0U)
        {
            SendStatus = _Transport.send(reinterpret_cast<const uint8_t *>(Request.Body),
                                         Request.BodyLength);
            if(SendStatus != TransportStatus::OK)
            {
                Status = HttpStatus::SEND_ERROR;
            }
        }
    }
    else
    {
        Status = HttpStatus::SEND_ERROR;
    }
    return Status;
}

HttpStatus HttpClient::receiveResponse(HttpResponse &Response, size_t &ResponseLength)
{
    HttpStatus Status = HttpStatus::OK;
    ResponseLength = 0U;
    bool NeedMoreData = true;

    while(NeedMoreData)
    {
        if(ResponseLength >= (_ResponseBuffer.size() - 1U))
        {
            Status = HttpStatus::RESPONSE_TOO_LONG;
            break;
        }

        size_t Received = 0;
        TransportStatus RecvStatus = _Transport.receive(
            reinterpret_cast<uint8_t *>(_ResponseBuffer.data() + ResponseLength),
            _ResponseBuffer.size() - ResponseLength - 1U, /* space for null terminator */
            Received);
        if(RecvStatus != TransportStatus::OK)
        {
            Status = HttpStatus::RECEIVE_ERROR;
            break;
        }
        if(Received == 0U)
        {
            Status = HttpStatus::MALFORMED_RESPONSE;
            break;
        }

        ResponseLength += Received;
        ReceiveDataState DataStatus = processReceivedData(ResponseLength, Response);
        if(DataStatus == ReceiveDataState::DATA_COMPLETE)
        {
            NeedMoreData = false;
        }
        else if(DataStatus == ReceiveDataState::MALFORMED_RESPONSE)
        {
            Status = HttpStatus::MALFORMED_RESPONSE;
            break;
        }
    }
    _ResponseBuffer[ResponseLength] = '\0';
    return Status;
}

HttpClient::ReceiveDataState HttpClient::processReceivedData(size_t DataLength,
                                                             HttpResponse &Response)
{
    ReceiveDataState Status = ReceiveDataState::NEED_MORE_DATA;
    const char *HeaderEnd = findHeaderEnd(_ResponseBuffer.data(), DataLength);
    if(HeaderEnd != nullptr)
    {
        HttpHeaderParseStatus ParseStatus =
            checkResponseBodyComplete(Response, *HeaderEnd, DataLength);
        if(ParseStatus != HttpHeaderParseStatus::DATA_NOT_RECEIVED)
        {
            ParseStatus == HttpHeaderParseStatus::OK
                ? Status = ReceiveDataState::DATA_COMPLETE
                : Status = ReceiveDataState::MALFORMED_RESPONSE;
        }
    }
    return Status;
}

HttpStatus HttpClient::buildRequestHeader(HttpRequest &Request)
{
    int Written = 0;
    if((Request.Method == HttpMethod::POST_METHOD) || (Request.Method == HttpMethod::PUT_METHOD))
    {
        Written = snprintf(_RequestHeader.data(),
                           _RequestHeader.size(),
                           HTTP_HEADER_WITH_BODY_FORMAT,
                           methodToString(Request.Method),
                           Request.Path,
                           _HostIp,
                           _Port,
                           USER_AGENT_STRING,
                           static_cast<unsigned>(Request.BodyLength));
    }
    else
    {
        Written = snprintf(_RequestHeader.data(),
                           _RequestHeader.size(),
                           HTTP_HEADER_FORMAT,
                           methodToString(Request.Method),
                           Request.Path,
                           _HostIp,
                           _Port,
                           USER_AGENT_STRING);
    }

    if((Written <= 0) || (static_cast<size_t>(Written) >= _RequestHeader.size()))
    {
        Request.HeaderLength = 0U;
        return HttpStatus::INTERNAL_ERROR;
    }
    Request.HeaderLength = static_cast<size_t>(Written);
    return HttpStatus::OK;
}

} //namespace Network
