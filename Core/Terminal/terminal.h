/**
 ******************************************************************************
 * @file        : terminal.h
 * @author      : embedom
 * @date        : 2026-05-01
 * @brief       : Terminal header file for segger rtt color logs.
 ******************************************************************************
 */

#ifndef CUSTOM_TERMINAL_H
#define CUSTOM_TERMINAL_H

#ifdef __cplusplus
extern "C"
{
#endif

    /********************************** CONFIG ***********************************/

#define LOG_LEVEL_NONE   0
#define LOG_LEVEL_ERROR  1
#define LOG_LEVEL_WARN   2
#define LOG_LEVEL_INFO   3
#define LOG_LEVEL_DEBUG  4

/* Log level for terminal output */
#define LOG_LEVEL LOG_LEVEL_DEBUG

    /********************************** DEFINES **********************************/

    typedef enum
    {
        LOG_TERMINAL_DEBUG,
        LOG_TERMINAL_INFO,
        LOG_TERMINAL_WARN,
        LOG_TERMINAL_ERROR
    } log_terminal_level_t;

    void terminal_print(log_terminal_level_t Color, const char *Tag, const char *FormatString, ...);

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define TERMINAL_LOG_ERROR(Tag, FormatString, ...) \
    terminal_print(LOG_TERMINAL_ERROR, Tag, FormatString, ##__VA_ARGS__)
#else
#define TERMINAL_LOG_ERROR(tag, fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define TERMINAL_LOG_WARN(Tag, FormatString, ...) \
    terminal_print(LOG_TERMINAL_WARN, Tag, FormatString, ##__VA_ARGS__)
#else
#define TERMINAL_LOG_WARN(tag, fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define TERMINAL_LOG_INFO(Tag, FormatString, ...) \
    terminal_print(LOG_TERMINAL_INFO, Tag, FormatString, ##__VA_ARGS__)
#else
#define TERMINAL_LOG_INFO(tag, fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define TERMINAL_LOG_DEBUG(Tag, FormatString, ...) \
    terminal_print(LOG_TERMINAL_DEBUG, Tag, FormatString, ##__VA_ARGS__)
#else
#define TERMINAL_LOG_DEBUG(tag, fmt, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* CUSTOM_TERMINAL_H */