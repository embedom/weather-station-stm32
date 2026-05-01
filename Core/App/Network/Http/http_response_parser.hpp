/**
 ******************************************************************************
 * @file        : http_response_parser.hpp
 * @author      : embedom
 * @date        : 2026-05-14
 * @brief       : Description
 ******************************************************************************
 */

#pragma once

/********************************* INCLUDES **********************************/

#include <cstdint>
#include "http_client.hpp"

namespace Network
{

/******************************** CONSTEXPR **********************************/

/********************************* TYPEDEFS **********************************/

enum class HttpHeaderParseStatus : uint8_t
{
    INVALID_HTTP_VERSION,
    INVALID_STATUS_CODE,
    INVALID_HEADER_END,
    INVALID_CONTENT_LENGTH,
    INVALID_BODY_LENGTH,
    INVALID_CONTENT_TYPE,
    OK
};

/*********************************** CLASS ***********************************/

HttpHeaderParseStatus parseHttpResponse(const char *ResponseBuffer, size_t ResponseLength,
                                        HttpResponse &Response);

} //namespace Network
