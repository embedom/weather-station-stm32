/**
 *****************************************************************************
 * @file        : misc_compiler.h
 * @author      : embedom
 * @date        : 2026-04-03
 * @brief       : Compiler and debug utility macros.
 *****************************************************************************
 */

#ifndef COMPILER_H
#define COMPILER_H

#include "cmsis_gcc.h"

/************************** COMPILER ATTRIBUTES ******************************/

#define NOTUSED(x)       ((void)(x))
#define WEAK            __attribute__((weak))
#define PACKED          __attribute__((packed))
#define ALIGNED(n)      __attribute__((aligned(n)))
#define NORETURN        __attribute__((noreturn))
#define SECTION(s)      __attribute__((section(s)))

/****************************** DEBUG MACROS *********************************/

#ifndef RELEASE
#define DEBUG_BRKPT()   __BKPT(0)
#else
#define DEBUG_BRKPT()   ((void)0)
#endif

#endif /* COMPILER_H */
