#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"

#define MODEL_ID_REGISTER              0xC0U
#define MODEL_ID_VALUE                 0xEEU
#define SYSTEM_SEQUENCE_CONFIG         0x01U
#define RESULT_INTERRUPT_STATUS        0x13U
#define RESULT_RANGE_MM                0x1EU
#define SYSRANGE_START                 0x00U
#define TUNING_SIGNATURE_REGISTER      0x24U
#define TUNING_SIGNATURE_VALUE         0x01U

typedef struct
{
    uint8_t reg;
    uint8_t value;
} Write_t;

static uint8_t s_registers[256];
static Write_t s_writes[256];
static unsigned int s_writeCount;
static TickType_t s_ticks;

QueueHandle_t rawdataQueue;

#include "../src/sensors/vl53l0x_sensor.c"

static void ResetDevice(uint8_t modelId)
{
    memset(s_registers, 0, sizeof(s_registers));
    memset(s_writes, 0, sizeof(s_writes));
    s_writeCount = 0U;
    s_ticks = 0U;
    rawdataQueue = NULL;
    s_registers[MODEL_ID_REGISTER] = modelId;
    s_registers[0x91U] = 0xA5U;
    s_registers[0x92U] = 0x85U;
    memset(&s_registers[0xB0U], 0xFF, 6U);
    s_registers[RESULT_INTERRUPT_STATUS] = 0x07U;
}

static bool SawWrite(uint8_t reg, uint8_t value)
{
    unsigned int i;

    for (i = 0U; i < s_writeCount; i++)
    {
        if (s_writes[i].reg == reg && s_writes[i].value == value)
        {
            return true;
        }
    }
    return false;
}

bool writeI2C_VL53L0X_Byte(uint8_t address, uint8_t reg, uint8_t value)
{
    assert(address == 0x29U);
    assert(s_writeCount < (sizeof(s_writes) / sizeof(s_writes[0])));
    s_writes[s_writeCount].reg = reg;
    s_writes[s_writeCount].value = value;
    s_writeCount++;
    s_registers[reg] = value;
    return true;
}

bool readI2C_VL53L0X(uint8_t address, uint8_t reg, uint8_t *data, uint16_t length)
{
    uint16_t i;

    assert(address == 0x29U);
    assert(data != NULL);

    if (reg == 0x83U)
    {
        data[0] = 0x01U;
        return true;
    }

    for (i = 0U; i < length; i++)
    {
        data[i] = s_registers[(uint8_t)(reg + i)];
    }
    return true;
}

TickType_t xTaskGetTickCount(void)
{
    return s_ticks++;
}

void vTaskDelay(TickType_t ticks)
{
    s_ticks += ticks;
}

BaseType_t xQueueSendToBack(QueueHandle_t queue,
                            const void *message,
                            TickType_t waitTicks)
{
    (void)queue;
    (void)message;
    (void)waitTicks;
    return pdTRUE;
}

int main(void)
{
    ResetDevice(0x00U);
    assert(!Vl53l0xSensor_Init());
    assert(s_writeCount == 0U);

    ResetDevice(MODEL_ID_VALUE);
    assert(Vl53l0xSensor_Init());
    assert(SawWrite(TUNING_SIGNATURE_REGISTER, TUNING_SIGNATURE_VALUE));
    assert(SawWrite(SYSTEM_SEQUENCE_CONFIG, 0x01U));
    assert(SawWrite(SYSTEM_SEQUENCE_CONFIG, 0x02U));
    assert(SawWrite(0x91U, 0xA5U));
    assert(SawWrite(SYSRANGE_START, 0x02U));

    return 0;
}
