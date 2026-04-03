/**
 ******************************************************************************
 * @file        : app_init.hpp
 * @author      : embedom
 * @date        : 2026-04-03
 * @brief       : App initialize header file.
 ******************************************************************************
 */

#pragma once

/********************************* INCLUDES **********************************/

#include <stdint.h>

namespace App
{

/********************************* TYPEDEFS **********************************/

enum class AppStatus : uint8_t
{
    OK = 0,
    ERROR = 1
};

/**************************** FUNCTION PROTOTYPES ****************************/

AppStatus platformInit(void);
AppStatus initialize(void);
void startOs(void);

} //namespace App
