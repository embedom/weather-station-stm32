/**
  *****************************************************************************
  * @file        : main.cpp
  * @author      : embedom
  * @date        : 2026-03-07
  * @brief       : App main source file.
  *****************************************************************************
  */

/********************************* INCLUDES **********************************/

#include <stdint.h>
#include "misc_compiler.h"
#include "app_init.hpp"

/********************************* FUNCTIONS *********************************/

int main(void)
{
    if(App::platformInit() != App::AppStatus::OK)
    {
        DEBUG_BRKPT();
        for(;;);
    }

    if(App::initialize() != App::AppStatus::OK)
    {
        DEBUG_BRKPT();
        for(;;);
    }
    App::startOs();
}
