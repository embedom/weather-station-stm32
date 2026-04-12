/**
  *****************************************************************************
  * @file        : generic_task.hpp
  * @author      : embedom
  * @date        : 2026-03-21
  * @brief       : Description
  *****************************************************************************
  */

#pragma once

/********************************* INCLUDES **********************************/

#include "FreeRTOS.h"
#include "task.h"

/******************************** CONSTEXPR **********************************/

/********************************* TYPEDEFS **********************************/

/*********************************** CLASS ***********************************/

class GenericTask
{
    public:
    struct StaticTaskConfig
    {
        const char *TaskName;
        configSTACK_DEPTH_TYPE StackSizeWords;
        UBaseType_t Priority;
        StackType_t *StackBuffer;
        StaticTask_t *TaskControlBlock;
    };

    GenericTask() = default;
    virtual ~GenericTask() = default;

    void createStaticTask(const StaticTaskConfig &TaskConfig)
    {
        configASSERT(_TaskHandle == nullptr);
        configASSERT(TaskConfig.TaskName != nullptr);
        configASSERT(TaskConfig.StackBuffer != nullptr);
        configASSERT(TaskConfig.TaskControlBlock != nullptr);

        _TaskHandle = xTaskCreateStatic(&GenericTask::taskEntry,
                                        TaskConfig.TaskName,
                                        TaskConfig.StackSizeWords,
                                        this,
                                        TaskConfig.Priority,
                                        TaskConfig.StackBuffer,
                                        TaskConfig.TaskControlBlock);

        configASSERT(_TaskHandle != nullptr);
    }

    protected:
    virtual void runCyclic() = 0;
    virtual void onTaskStartUp() = 0;

    private:
    TaskHandle_t _TaskHandle = nullptr;

    static void taskEntry(void *Arg)
    {
        GenericTask *TaskInstance = static_cast<GenericTask *>(Arg);
        TaskInstance->onTaskStartUp();
        TaskInstance->runCyclic();

        /* Safety fallback: task implementation should never return from runCyclic. */
        vTaskDelete(nullptr);
    }

}; //GenericTask
