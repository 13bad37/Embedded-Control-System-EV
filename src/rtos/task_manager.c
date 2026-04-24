#include "rtos/task_manager.h"

#include "common/project_config.h"
#include "rtos/tasks.h"
#include "task.h"

static void MarkFailure(BaseType_t result, BaseType_t *overallStatus)
{
    if (result != pdPASS)
    {
        *overallStatus = pdFAIL;
    }
}

BaseType_t RtosTasks_CreateAll(void)
{
    BaseType_t status = pdPASS;

    MarkFailure(xTaskCreate(MotorControlTask,
                            "motor",
                            PROJECT_TASK_STACK_WORDS_MEDIUM,
                            NULL,
                            PROJECT_TASK_PRIORITY_MOTOR,
                            NULL),
                &status);

    MarkFailure(xTaskCreate(SensorTask,
                            "sensors",
                            PROJECT_TASK_STACK_WORDS_MEDIUM,
                            NULL,
                            PROJECT_TASK_PRIORITY_SENSOR,
                            NULL),
                &status);

    MarkFailure(xTaskCreate(GuiTask,
                            "gui",
                            PROJECT_TASK_STACK_WORDS_SMALL,
                            NULL,
                            PROJECT_TASK_PRIORITY_GUI,
                            NULL),
                &status);

    MarkFailure(xTaskCreate(FaultTask,
                            "fault",
                            PROJECT_TASK_STACK_WORDS_SMALL,
                            NULL,
                            PROJECT_TASK_PRIORITY_FAULT,
                            NULL),
                &status);

    MarkFailure(xTaskCreate(IntegrationTask,
                            "integrate",
                            PROJECT_TASK_STACK_WORDS_SMALL,
                            NULL,
                            PROJECT_TASK_PRIORITY_INTEGRATION,
                            NULL),
                &status);

    MarkFailure(xTaskCreate(SerialPlotTask,
                            "serialplot",
                            PROJECT_TASK_STACK_WORDS_SMALL,
                            NULL,
                            PROJECT_TASK_PRIORITY_TELEMETRY,
                            NULL),
                &status);

    return status;
}
