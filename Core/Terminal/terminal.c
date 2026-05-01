/**
 ******************************************************************************
 * @file        : terminal.c
 * @author      : embedom
 * @date        : 2026-05-01
 * @brief       : Terminal logs with colors and FreeRTOS timestamp.
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <stdint.h>
#include <stdarg.h>
#include "terminal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "SEGGER_RTT.h"

/********************************* DEFINES ***********************************/

#define LOG_TERMINAL_INFO_COLOR   "\x1b[34m" // Blue
#define LOG_TERMINAL_DEBUG_COLOR  "\x1b[35m" // Magenta
#define LOG_TERMINAL_WARN_COLOR   "\x1b[33m" // Yellow
#define LOG_TERMINAL_ERROR_COLOR  "\x1b[31m" // Red
#define LOG_TERMINAL_RESET_COLOR  "\x1b[0m"

/********************************* FUNCTIONS *********************************/

uint32_t getTerminalTimeMs(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

const char *getTerminalLogColor(log_terminal_level_t Color)
{
    switch(Color)
    {
    case LOG_TERMINAL_INFO:
        return LOG_TERMINAL_INFO_COLOR;
    case LOG_TERMINAL_DEBUG:
        return LOG_TERMINAL_DEBUG_COLOR;
    case LOG_TERMINAL_WARN:
        return LOG_TERMINAL_WARN_COLOR;
    case LOG_TERMINAL_ERROR:
        return LOG_TERMINAL_ERROR_COLOR;
    default:
        return LOG_TERMINAL_RESET_COLOR;
    }
}

void terminal_print(log_terminal_level_t Color, const char *Tag, const char *FormatString, ...)
{
    va_list Args;
    va_start(Args, FormatString);

    SEGGER_RTT_printf(0, "%s[%lu] [%s] - ", getTerminalLogColor(Color), getTerminalTimeMs(), Tag);
    SEGGER_RTT_vprintf(0, FormatString, &Args);
    SEGGER_RTT_printf(0, LOG_TERMINAL_RESET_COLOR "\n");
    va_end(Args);
}
