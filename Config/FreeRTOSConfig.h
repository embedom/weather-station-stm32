/*
 * FreeRTOS Kernel
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configCPU_CLOCK_HZ    ( ( unsigned long ) 216000000 )

// #define configSYSTICK_CLOCK_HZ                  1000


/* Scheduling behaviour related definitions. **********************************/

#define configTICK_RATE_HZ                         500

#define configUSE_PREEMPTION                       1

#define configUSE_TIME_SLICING                     0

#define configUSE_PORT_OPTIMISED_TASK_SELECTION    1

#define configUSE_TICKLESS_IDLE                    0

#define configMAX_PRIORITIES                       10

#define configMINIMAL_STACK_SIZE                   128

#define configMAX_TASK_NAME_LEN                    16

#define configTICK_TYPE_WIDTH_IN_BITS              TICK_TYPE_WIDTH_32_BITS

#define configIDLE_SHOULD_YIELD                    1

#define configTASK_NOTIFICATION_ARRAY_ENTRIES      1

#define configQUEUE_REGISTRY_SIZE                  10

#define configENABLE_BACKWARD_COMPATIBILITY        0

#define configNUM_THREAD_LOCAL_STORAGE_POINTERS    0

#define configUSE_MINI_LIST_ITEM                   0

#define configSTACK_DEPTH_TYPE                     size_t

#define configMESSAGE_BUFFER_LENGTH_TYPE           size_t

#define configHEAP_CLEAR_MEMORY_ON_FREE            0

#define configSTATS_BUFFER_MAX_LENGTH              0xFFFF

#define configUSE_NEWLIB_REENTRANT                 0


/* Software timer related definitions. ****************************************/

#define configUSE_TIMERS                1

#define configTIMER_TASK_PRIORITY       ( configMAX_PRIORITIES - 1 )

#define configTIMER_TASK_STACK_DEPTH    configMINIMAL_STACK_SIZE

#define configTIMER_QUEUE_LENGTH        10


/* Event Group related definitions. *******************************************/

#define configUSE_EVENT_GROUPS    1

#define configUSE_STREAM_BUFFERS    1


/* Memory allocation related definitions. *************************************/

#define configKERNEL_PROVIDED_STATIC_MEMORY          1

#define configSUPPORT_STATIC_ALLOCATION              1

#define configSUPPORT_DYNAMIC_ALLOCATION             1

#define configTOTAL_HEAP_SIZE                        ( ( size_t ) ( 10 * 1024 ) )

#define configAPPLICATION_ALLOCATED_HEAP             0

#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP    0

#define configENABLE_HEAP_PROTECTOR                  0


/* Interrupt nesting behaviour configuration. *********************************/

#define configPRIO_BITS                               4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY          ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )


/* Hook and callback function related definitions. ****************************/

#define configUSE_IDLE_HOOK                   0
#define configUSE_TICK_HOOK                   0
#define configUSE_MALLOC_FAILED_HOOK          0
#define configUSE_DAEMON_TASK_STARTUP_HOOK    0

#define configUSE_SB_COMPLETED_CALLBACK       0

#define configCHECK_FOR_STACK_OVERFLOW        2


/* Run time and task stats gathering related definitions. *********************/

#define configGENERATE_RUN_TIME_STATS           0

#if (configGENERATE_RUN_TIME_STATS == 1)
extern void vConfigureRunTimeStatsTimer( void );
extern uint32_t ulGetRunTimeCounterValue( void );
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() vConfigureRunTimeStatsTimer()
#define portGET_RUN_TIME_COUNTER_VALUE()         ulGetRunTimeCounterValue()
#endif

#define configUSE_STATS_FORMATTING_FUNCTIONS    0

#define configUSE_TRACE_FACILITY                1

#define configRECORD_STACK_HIGH_ADDRESS         1


/* Co-routine related definitions. ********************************************/

#define configUSE_CO_ROUTINES              0

#define configMAX_CO_ROUTINE_PRIORITIES    1


/* Debugging assistance. ******************************************************/

/* *INDENT-OFF* */
#define configASSERT( x )         \
    if( ( x ) == 0 )              \
    {                             \
        taskDISABLE_INTERRUPTS(); \
        for( ; ; )                \
        ;                         \
    }
/* *INDENT-ON* */


/* FreeRTOS MPU specific definitions. *****************************************/

#ifdef USE_ARM_MPU_PORT

#define portUSING_MPU_WRAPPERS                                    0

#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS    0

#define configTOTAL_MPU_REGIONS                                   8

#define configTEX_S_C_B_FLASH                                     0x07UL

#define configTEX_S_C_B_SRAM                                      0x07UL

#define configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY               1

#define configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS                0

#define configUSE_MPU_WRAPPERS_V1                                 0

#define configPROTECTED_KERNEL_OBJECT_POOL_SIZE                   10

#define configSYSTEM_CALL_STACK_SIZE                              128

#define configENABLE_ACCESS_CONTROL_LIST                          1

#endif /* USE_ARM_MPU_PORT */

/* ARMv8-M port Specific Configuration definitions. ***************************/

#define configENABLE_TRUSTZONE              0

#define configRUN_FREERTOS_SECURE_ONLY      0

#define configENABLE_MPU                    0

#define configENABLE_FPU                    1

#define configENABLE_MVE                    0

#define configCHECK_HANDLER_INSTALLATION    1


/* Definitions that include or exclude functionality. *************************/

#define configUSE_TASK_NOTIFICATIONS           1
#define configUSE_MUTEXES                      1
#define configUSE_RECURSIVE_MUTEXES            1
#define configUSE_COUNTING_SEMAPHORES          1
#define configUSE_QUEUE_SETS                   0
#define configUSE_APPLICATION_TASK_TAG         0

#define configUSE_POSIX_ERRNO                  0

#define INCLUDE_vTaskPrioritySet               1
#define INCLUDE_uxTaskPriorityGet              1
#define INCLUDE_vTaskDelete                    1
#define INCLUDE_vTaskSuspend                   1
#define INCLUDE_xTaskDelayUntil                1
#define INCLUDE_vTaskDelay                     1
#define INCLUDE_xTaskGetSchedulerState         1
#define INCLUDE_xTaskGetCurrentTaskHandle      1
#define INCLUDE_uxTaskGetStackHighWaterMark    1
#define INCLUDE_xTaskGetIdleTaskHandle         0
#define INCLUDE_eTaskGetState                  0
#define INCLUDE_xTimerPendFunctionCall         0
#define INCLUDE_xTaskAbortDelay                0
#define INCLUDE_xTaskGetHandle                 0
#define INCLUDE_xTaskResumeFromISR             1

/* Cortex-M exception handlers used by the GCC/ARM_CM7 FreeRTOS port. */
#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler
#define xPortSysTickHandler                     SysTick_Handler

#endif /* FREERTOS_CONFIG_H */