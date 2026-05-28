/**
 ******************************************************************************
 * @file        : http_response_parser.cpp
 * @author      : embedom
 * @date        : 2026-05-14
 * @brief       : Description
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <stdio.h>
#include <cstdint>
#include <string.h>

#include "terminal.h"
#include "app_config.hpp"
#include "http_client.hpp"

namespace Network
{

/******************************** CONSTEXPR **********************************/

constexpr size_t HTTP_STATUS_LINE_MIN_LEN = 12U; /* "HTTP/1.1 200" */
constexpr size_t HTTP_VERSION_LEN = 9U;          /* "HTTP/1.1 " */
constexpr size_t HTTP_HEADER_END_LEN = 4U;       /* "\r\n\r\n" */
constexpr size_t HTTP_STATUS_CODE_LEN = 3U;
constexpr int HTTP_DECIMAL_BASE = 10;

constexpr char HTTP_VERSION[] = "HTTP/1.1 ";
constexpr char HTTP_HEADER_END_SEPARATOR[] = "\r\n\r\n";
constexpr char HTTP_LINE_END_SEPARATOR[] = "\r\n";
constexpr char HTTP_CONTENT_LENGTH_HEADER[] = "Content-Length";
constexpr char HTTP_CONTENT_TYPE_HEADER[] = "Content-Type";
constexpr char HTTP_JSON_CONTENT_TYPE[] = "application/json";

/****************************** STATIC HELPERS *******************************/

static char toLowerAscii(char CharValue);
static bool isDigit(char CharValue);
static const char *findSeparatorInRange(const char *Begin, const char *End, const char *Pattern,
                                        size_t PatternLen);
static bool equalsIgnoreCase(const char *Left, size_t LeftLen, const char *Right, size_t RightLen);
static void trimHeaderValue(const char *&ValueStart, const char *&ValueEnd);
static bool findHeaderValue(const char *HeadersStart, const char *HeadersEnd,
                            const char *HeaderName, size_t HeaderNameLen, const char *&ValueStart,
                            const char *&ValueEnd);
static bool parseContentLength(const char *ValueStart, const char *ValueEnd, size_t &ContentLength);
static bool isJsonContentType(const char *ValueStart, const char *ValueEnd);

/********************************** PUBLIC ***********************************/

const char *HttpClient::findHeaderEnd(const char *ResponseBuffer, size_t ResponseLength)
{
    return findSeparatorInRange(ResponseBuffer,
                                ResponseBuffer + ResponseLength,
                                HTTP_HEADER_END_SEPARATOR,
                                strlen(HTTP_HEADER_END_SEPARATOR));
}

HttpClient::HttpHeaderParseStatus HttpClient::checkResponseBodyComplete(HttpResponse &Response,
                                                                        size_t HeaderEndOffset,
                                                                        size_t ResponseLength)
{
    HttpHeaderParseStatus Status = HttpHeaderParseStatus::OK;
    const char *HeaderEnd = _ResponseBuffer.data() + HeaderEndOffset;
    const char *BodyStart = HeaderEnd + HTTP_HEADER_END_LEN;
    const size_t BodyStartOffset = static_cast<size_t>(BodyStart - _ResponseBuffer.data());
    const char *ContentLengthStart = nullptr;
    const char *ContentLengthEnd = nullptr;

    bool HasContentLength = findHeaderValue(_ResponseBuffer.data(),
                                            HeaderEnd,
                                            HTTP_CONTENT_LENGTH_HEADER,
                                            strlen(HTTP_CONTENT_LENGTH_HEADER),
                                            ContentLengthStart,
                                            ContentLengthEnd);
    if(HasContentLength)
    {
        size_t ExpectedContentLength = 0U;
        size_t ReceivedBodyLength = ResponseLength - BodyStartOffset;
        bool ContentLengthValid =
            parseContentLength(ContentLengthStart, ContentLengthEnd, ExpectedContentLength);
        if(!ContentLengthValid)
        {
            Status = HttpHeaderParseStatus::INVALID_CONTENT_LENGTH;
        }
        else if(ReceivedBodyLength > ExpectedContentLength)
        {
            Status = HttpHeaderParseStatus::INVALID_BODY_LENGTH;
        }
        else if(ReceivedBodyLength < ExpectedContentLength)
        {
            Status = HttpHeaderParseStatus::DATA_NOT_RECEIVED;
        }
        else
        {
            Response.Body = BodyStart;
            Response.BodyLength = ExpectedContentLength;
            Status = HttpHeaderParseStatus::OK;
        }
    }
    else
    {
        /* No Content-Length header */
        Response.Body = nullptr;
        Response.BodyLength = 0U;
        Status = HttpHeaderParseStatus::OK;
    }
    return Status;
}

HttpClient::HttpHeaderParseStatus
HttpClient::validateHttpResponseHeaders(const char *ResponseBuffer, size_t ResponseLength,
                                        HttpResponse &Response)
{
    if((ResponseLength < HTTP_STATUS_LINE_MIN_LEN) ||
       (memcmp(ResponseBuffer, HTTP_VERSION, HTTP_VERSION_LEN) != 0))
    {
        return HttpHeaderParseStatus::INVALID_HTTP_VERSION;
    }

    const char *ResponseEnd = ResponseBuffer + ResponseLength;
    const char *StatusCodeStart = ResponseBuffer + HTTP_VERSION_LEN;
    const char *StatusCodeEnd = StatusCodeStart + HTTP_STATUS_CODE_LEN;
    /* Check if the status code is valid */
    if((StatusCodeEnd >= ResponseEnd) || !isDigit(StatusCodeStart[0]) ||
       !isDigit(StatusCodeStart[1]) || !isDigit(StatusCodeStart[2]) ||
       ((*StatusCodeEnd != ' ') && (*StatusCodeEnd != '\r')))
    {
        return HttpHeaderParseStatus::INVALID_STATUS_CODE;
    }
    const int StatusCode = ((StatusCodeStart[0] - '0') * 100) + ((StatusCodeStart[1] - '0') * 10) +
                           (StatusCodeStart[2] - '0');

    const char *HeaderEnd = findSeparatorInRange(
        ResponseBuffer, ResponseEnd, HTTP_HEADER_END_SEPARATOR, strlen(HTTP_HEADER_END_SEPARATOR));
    if(HeaderEnd == nullptr)
    {
        return HttpHeaderParseStatus::INVALID_HEADER_END;
    }

    const char *StatusLineEnd = findSeparatorInRange(
        ResponseBuffer, HeaderEnd, HTTP_LINE_END_SEPARATOR, strlen(HTTP_LINE_END_SEPARATOR));
    if(StatusLineEnd == nullptr)
    {
        return HttpHeaderParseStatus::INVALID_STATUS_CODE;
    }
    const char *HeadersStart = StatusLineEnd + strlen(HTTP_LINE_END_SEPARATOR);

    if(Response.BodyLength > 0U)
    {
        const char *ContentTypeStart = nullptr;
        const char *ContentTypeEnd = nullptr;
        bool HasContentType = findHeaderValue(HeadersStart,
                                              HeaderEnd,
                                              HTTP_CONTENT_TYPE_HEADER,
                                              strlen(HTTP_CONTENT_TYPE_HEADER),
                                              ContentTypeStart,
                                              ContentTypeEnd);
        bool IsJson = isJsonContentType(ContentTypeStart, ContentTypeEnd);
        if(!HasContentType || !IsJson)
        {
            return HttpHeaderParseStatus::INVALID_CONTENT_TYPE;
        }
    }
#ifdef HTTP_RESPONSE_PARSER_DEBUG
    /* Debug output of the parsed response */
    TERMINAL_LOG_DEBUG("HttpClient", "Parsed HTTP response:");
    TERMINAL_LOG_DEBUG("HttpClient",
                       "Status code: %d, Header length: %u, Body length: %u",
                       StatusCode,
                       static_cast<size_t>(HeaderEnd - ResponseBuffer),
                       static_cast<size_t>(Response.BodyLength));
#endif
    Response.Status = HttpStatus::OK;
    Response.StatusCode = StatusCode;
    return HttpHeaderParseStatus::OK;
}

/****************************** STATIC HELPERS *******************************/

static char toLowerAscii(char CharValue)
{
    if((CharValue >= 'A') && (CharValue <= 'Z'))
    {
        return static_cast<char>(CharValue + ('a' - 'A'));
    }
    return CharValue;
}

static bool isDigit(char CharValue)
{
    return (CharValue >= '0') && (CharValue <= '9');
}

static const char *findSeparatorInRange(const char *Begin, const char *End, const char *Pattern,
                                        size_t PatternLen)
{
    if((PatternLen == 0U) || (Begin >= End))
    {
        return nullptr;
    }
    const size_t RangeLen = static_cast<size_t>(End - Begin);
    if(RangeLen < PatternLen)
    {
        return nullptr;
    }

    const char *Cursor = Begin;
    const char *Last = End - PatternLen;
    while(Cursor <= Last)
    {
        if(Cursor[0] == Pattern[0])
        {
            bool Match = true;
            for(size_t Index = 1U; Index < PatternLen; ++Index)
            {
                if(Cursor[Index] != Pattern[Index])
                {
                    Match = false;
                    break;
                }
            }
            if(Match)
            {
                return Cursor;
            }
        }
        ++Cursor;
    }
    return nullptr;
}

static bool equalsIgnoreCase(const char *Left, size_t LeftLen, const char *Right, size_t RightLen)
{
    if(LeftLen != RightLen)
    {
        return false;
    }

    for(size_t Index = 0U; Index < LeftLen; ++Index)
    {
        if(toLowerAscii(Left[Index]) != toLowerAscii(Right[Index]))
        {
            return false;
        }
    }
    return true;
}

static void trimHeaderValue(const char *&ValueStart, const char *&ValueEnd)
{
    while((ValueStart < ValueEnd) && ((*ValueStart == ' ') || (*ValueStart == '\t')))
    {
        ++ValueStart;
    }
    while((ValueEnd > ValueStart) && ((ValueEnd[-1] == ' ') || (ValueEnd[-1] == '\t')))
    {
        --ValueEnd;
    }
}

static bool findHeaderValue(const char *HeadersStart, const char *HeadersEnd,
                            const char *HeaderName, size_t HeaderNameLen, const char *&ValueStart,
                            const char *&ValueEnd)
{
    const char *LineStart = HeadersStart;
    while(LineStart < HeadersEnd)
    {
        const char *LineEnd = findSeparatorInRange(
            LineStart, HeadersEnd, HTTP_LINE_END_SEPARATOR, strlen(HTTP_LINE_END_SEPARATOR));
        if(LineEnd == nullptr)
        {
            LineEnd = HeadersEnd;
        }

        const char *Colon = static_cast<const char *>(
            memchr(LineStart, ':', static_cast<size_t>(LineEnd - LineStart)));
        if((Colon != nullptr) &&
           equalsIgnoreCase(
               LineStart, static_cast<size_t>(Colon - LineStart), HeaderName, HeaderNameLen))
        {
            ValueStart = Colon + 1U;
            ValueEnd = LineEnd;
            trimHeaderValue(ValueStart, ValueEnd);
            return true;
        }

        if(LineEnd == HeadersEnd)
        {
            break;
        }
        LineStart = LineEnd + strlen(HTTP_LINE_END_SEPARATOR);
    }

    return false;
}

static bool parseContentLength(const char *ValueStart, const char *ValueEnd, size_t &ContentLength)
{
    if(ValueStart == ValueEnd)
    {
        return false;
    }

    size_t Value = 0U;
    for(const char *Cursor = ValueStart; Cursor < ValueEnd; ++Cursor)
    {
        if(!isDigit(*Cursor))
        {
            return false;
        }

        const size_t Digit = static_cast<size_t>(*Cursor - '0');
        if(Value > ((HTTP_MAX_RESPONSE_LEN - Digit) / static_cast<size_t>(HTTP_DECIMAL_BASE)))
        {
            return false;
        }
        Value = (Value * static_cast<size_t>(HTTP_DECIMAL_BASE)) + Digit;
    }
    ContentLength = Value;
    return true;
}

static bool isJsonContentType(const char *ValueStart, const char *ValueEnd)
{
    const char *TypeEnd = ValueStart;
    while((TypeEnd < ValueEnd) && (*TypeEnd != ';'))
    {
        ++TypeEnd;
    }
    trimHeaderValue(ValueStart, TypeEnd);

    return equalsIgnoreCase(ValueStart,
                            static_cast<size_t>(TypeEnd - ValueStart),
                            HTTP_JSON_CONTENT_TYPE,
                            strlen(HTTP_JSON_CONTENT_TYPE));
}

} //namespace Network
