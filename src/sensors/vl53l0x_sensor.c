#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "sensors/i2c2ShtDriver.h"
#include "sensors/vl53l0x_sensor.h"
#include "sensors/sensor_manager.h"

#define VL53L0X_I2C_ADDRESS                         0x29U
#define VL53L0X_EXPECTED_MODEL_ID                   0xEEU

#define VL53L0X_REG_SYSRANGE_START                  0x00U
#define VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG          0x01U
#define VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO    0x0AU
#define VL53L0X_REG_INTERRUPT_CLEAR                 0x0BU
#define VL53L0X_REG_RESULT_INTERRUPT                0x13U
#define VL53L0X_REG_RESULT_RANGE_MM                 0x1EU
#define VL53L0X_REG_FINAL_RANGE_MIN_COUNT_RATE      0x44U
#define VL53L0X_REG_MSRC_CONFIG_CONTROL             0x60U
#define VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH         0x84U
#define VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA          0x89U
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID         0xC0U

#define VL53L0X_REG_DYNAMIC_SPAD_NUM_REQUESTED      0x4EU
#define VL53L0X_REG_DYNAMIC_SPAD_START_OFFSET       0x4FU
#define VL53L0X_REG_PRE_RANGE_VCSEL_PERIOD          0x50U
#define VL53L0X_REG_PRE_RANGE_TIMEOUT_HI            0x51U
#define VL53L0X_REG_MSRC_TIMEOUT                    0x46U
#define VL53L0X_REG_GLOBAL_REF_ENABLE_START         0xB6U
#define VL53L0X_REG_GLOBAL_SPAD_ENABLES_REF_0       0xB0U
#define VL53L0X_REG_FINAL_RANGE_VCSEL_PERIOD        0x70U
#define VL53L0X_REG_FINAL_RANGE_TIMEOUT_HI          0x71U

#define VL53L0X_START_CONTINUOUS                    0x02U
#define VL53L0X_CLEAR_INTERRUPT                     0x01U
#define VL53L0X_NEW_SAMPLE_MASK                     0x07U
#define VL53L0X_SAMPLE_PERIOD_MS                    20U
#define VL53L0X_INIT_TIMEOUT_MS                     500U
#define VL53L0X_INVALID_DISTANCE_MM                 8190U
#define VL53L0X_CLOSE_DISTANCE_MM                   1U

typedef struct
{
    uint8_t reg;
    uint8_t value;
} Vl53l0xRegValue_t;

typedef struct
{
    bool tcc;
    bool dss;
    bool msrc;
    bool preRange;
    bool finalRange;
} Vl53l0xSequenceEnables_t;

typedef struct
{
    uint8_t preRangeVcselPeriodPclks;
    uint8_t finalRangeVcselPeriodPclks;
    uint16_t msrcDssTccMclks;
    uint16_t preRangeMclks;
    uint16_t finalRangeMclks;
    uint32_t msrcDssTccUs;
    uint32_t preRangeUs;
    uint32_t finalRangeUs;
} Vl53l0xSequenceTimeouts_t;

/*
 * ST's default tuning settings, used after DataInit/StaticInit. The old code
 * skipped all of this and asked a cold sensor for ranges straight away.
 */
static const Vl53l0xRegValue_t s_defaultTuningSettings[] = {
    {0xFFU, 0x01U}, {0x00U, 0x00U}, {0xFFU, 0x00U}, {0x09U, 0x00U},
    {0x10U, 0x00U}, {0x11U, 0x00U}, {0x24U, 0x01U}, {0x25U, 0xFFU},
    {0x75U, 0x00U}, {0xFFU, 0x01U}, {0x4EU, 0x2CU}, {0x48U, 0x00U},
    {0x30U, 0x20U}, {0xFFU, 0x00U}, {0x30U, 0x09U}, {0x54U, 0x00U},
    {0x31U, 0x04U}, {0x32U, 0x03U}, {0x40U, 0x83U}, {0x46U, 0x25U},
    {0x60U, 0x00U}, {0x27U, 0x00U}, {0x50U, 0x06U}, {0x51U, 0x00U},
    {0x52U, 0x96U}, {0x56U, 0x08U}, {0x57U, 0x30U}, {0x61U, 0x00U},
    {0x62U, 0x00U}, {0x64U, 0x00U}, {0x65U, 0x00U}, {0x66U, 0xA0U},
    {0xFFU, 0x01U}, {0x22U, 0x32U}, {0x47U, 0x14U}, {0x49U, 0xFFU},
    {0x4AU, 0x00U}, {0xFFU, 0x00U}, {0x7AU, 0x0AU}, {0x7BU, 0x00U},
    {0x78U, 0x21U}, {0xFFU, 0x01U}, {0x23U, 0x34U}, {0x42U, 0x00U},
    {0x44U, 0xFFU}, {0x45U, 0x26U}, {0x46U, 0x05U}, {0x40U, 0x40U},
    {0x0EU, 0x06U}, {0x20U, 0x1AU}, {0x43U, 0x40U}, {0xFFU, 0x00U},
    {0x34U, 0x03U}, {0x35U, 0x44U}, {0xFFU, 0x01U}, {0x31U, 0x04U},
    {0x4BU, 0x09U}, {0x4CU, 0x05U}, {0x4DU, 0x04U}, {0xFFU, 0x00U},
    {0x44U, 0x00U}, {0x45U, 0x20U}, {0x47U, 0x08U}, {0x48U, 0x28U},
    {0x67U, 0x00U}, {0x70U, 0x04U}, {0x71U, 0x01U}, {0x72U, 0xFEU},
    {0x76U, 0x00U}, {0x77U, 0x00U}, {0xFFU, 0x01U}, {0x0DU, 0x01U},
    {0xFFU, 0x00U}, {0x80U, 0x01U}, {0x01U, 0xF8U}, {0xFFU, 0x01U},
    {0x8EU, 0x01U}, {0x00U, 0x01U}, {0xFFU, 0x00U}, {0x80U, 0x00U}
};

static TickType_t s_lastSampleTick;
static uint8_t s_stopVariable;

static bool WriteRegister(uint8_t reg, uint8_t value)
{
    return writeI2C_VL53L0X_Byte(VL53L0X_I2C_ADDRESS, reg, value);
}

static bool ReadRegister(uint8_t reg, uint8_t *value)
{
    return readI2C_VL53L0X(VL53L0X_I2C_ADDRESS, reg, value, 1U);
}

static bool WriteRegister16(uint8_t reg, uint16_t value)
{
    return WriteRegister(reg, (uint8_t)(value >> 8)) &&
           WriteRegister((uint8_t)(reg + 1U), (uint8_t)value);
}

static bool ReadRegister16(uint8_t reg, uint16_t *value)
{
    uint8_t bytes[2];

    if (!readI2C_VL53L0X(VL53L0X_I2C_ADDRESS, reg, bytes, 2U))
    {
        return false;
    }

    *value = ((uint16_t)bytes[0] << 8) | bytes[1];
    return true;
}

static bool WriteSettings(const Vl53l0xRegValue_t *settings, uint32_t count)
{
    uint32_t i;

    for (i = 0U; i < count; i++)
    {
        if (!WriteRegister(settings[i].reg, settings[i].value))
        {
            return false;
        }
    }
    return true;
}

static bool WaitForNewSample(void)
{
    TickType_t start = xTaskGetTickCount();
    uint8_t status;

    for (;;)
    {
        if (!ReadRegister(VL53L0X_REG_RESULT_INTERRUPT, &status))
        {
            return false;
        }
        if ((status & VL53L0X_NEW_SAMPLE_MASK) != 0U)
        {
            return true;
        }
        if ((xTaskGetTickCount() - start) >= pdMS_TO_TICKS(VL53L0X_INIT_TIMEOUT_MS))
        {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(1U));
    }
}

static bool GetSpadInfo(uint8_t *count, bool *typeIsAperture)
{
    uint8_t value;
    TickType_t start;

    if (!WriteRegister(0x80U, 0x01U) ||
        !WriteRegister(0xFFU, 0x01U) ||
        !WriteRegister(0x00U, 0x00U) ||
        !WriteRegister(0xFFU, 0x06U) ||
        !ReadRegister(0x83U, &value) ||
        !WriteRegister(0x83U, (uint8_t)(value | 0x04U)) ||
        !WriteRegister(0xFFU, 0x07U) ||
        !WriteRegister(0x81U, 0x01U) ||
        !WriteRegister(0x80U, 0x01U) ||
        !WriteRegister(0x94U, 0x6BU) ||
        !WriteRegister(0x83U, 0x00U))
    {
        return false;
    }

    start = xTaskGetTickCount();
    do
    {
        if (!ReadRegister(0x83U, &value))
        {
            return false;
        }
        if (value != 0U)
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1U));
    } while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(VL53L0X_INIT_TIMEOUT_MS));

    if (value == 0U || !WriteRegister(0x83U, 0x01U) ||
        !ReadRegister(0x92U, &value))
    {
        return false;
    }

    *count = value & 0x7FU;
    *typeIsAperture = (value & 0x80U) != 0U;

    return WriteRegister(0x81U, 0x00U) &&
           WriteRegister(0xFFU, 0x06U) &&
           ReadRegister(0x83U, &value) &&
           WriteRegister(0x83U, (uint8_t)(value & (uint8_t)~0x04U)) &&
           WriteRegister(0xFFU, 0x01U) &&
           WriteRegister(0x00U, 0x01U) &&
           WriteRegister(0xFFU, 0x00U) &&
           WriteRegister(0x80U, 0x00U);
}

static bool ConfigureReferenceSpads(void)
{
    uint8_t map[6];
    uint8_t count = 0U;
    uint8_t enabled = 0U;
    uint8_t first;
    uint8_t i;
    bool isAperture = false;

    if (!GetSpadInfo(&count, &isAperture) ||
        !readI2C_VL53L0X(VL53L0X_I2C_ADDRESS,
                          VL53L0X_REG_GLOBAL_SPAD_ENABLES_REF_0,
                          map,
                          (uint16_t)sizeof(map)) ||
        !WriteRegister(0xFFU, 0x01U) ||
        !WriteRegister(VL53L0X_REG_DYNAMIC_SPAD_START_OFFSET, 0x00U) ||
        !WriteRegister(VL53L0X_REG_DYNAMIC_SPAD_NUM_REQUESTED, 0x2CU) ||
        !WriteRegister(0xFFU, 0x00U) ||
        !WriteRegister(VL53L0X_REG_GLOBAL_REF_ENABLE_START, 0xB4U))
    {
        return false;
    }

    first = isAperture ? 12U : 0U;
    for (i = 0U; i < 48U; i++)
    {
        if (i < first || enabled == count)
        {
            map[i / 8U] &= (uint8_t)~(1U << (i % 8U));
        }
        else if ((map[i / 8U] & (uint8_t)(1U << (i % 8U))) != 0U)
        {
            enabled++;
        }
    }

    for (i = 0U; i < (uint8_t)sizeof(map); i++)
    {
        if (!WriteRegister((uint8_t)(VL53L0X_REG_GLOBAL_SPAD_ENABLES_REF_0 + i),
                           map[i]))
        {
            return false;
        }
    }
    return true;
}

static uint8_t DecodeVcselPeriod(uint8_t regValue)
{
    return (uint8_t)((regValue + 1U) << 1U);
}

static uint32_t CalcMacroPeriodNs(uint8_t vcselPeriodPclks)
{
    return (((uint32_t)2304U * vcselPeriodPclks * 1655U) + 500U) / 1000U;
}

static uint16_t DecodeTimeout(uint16_t encoded)
{
    return (uint16_t)(((encoded & 0x00FFU) << (encoded >> 8)) + 1U);
}

static uint16_t EncodeTimeout(uint32_t timeoutMclks)
{
    uint32_t lowByte;
    uint16_t highByte = 0U;

    if (timeoutMclks == 0U)
    {
        return 0U;
    }

    lowByte = timeoutMclks - 1U;
    while ((lowByte & 0xFFFFFF00UL) != 0U)
    {
        lowByte >>= 1U;
        highByte++;
    }
    return (uint16_t)((highByte << 8U) | (lowByte & 0xFFU));
}

static uint32_t TimeoutMclksToUs(uint16_t timeoutMclks, uint8_t vcselPeriodPclks)
{
    uint32_t periodNs = CalcMacroPeriodNs(vcselPeriodPclks);
    return (((uint32_t)timeoutMclks * periodNs) + 500U) / 1000U;
}

static uint32_t TimeoutUsToMclks(uint32_t timeoutUs, uint8_t vcselPeriodPclks)
{
    uint32_t periodNs = CalcMacroPeriodNs(vcselPeriodPclks);
    return ((timeoutUs * 1000U) + (periodNs / 2U)) / periodNs;
}

static bool GetSequenceEnables(Vl53l0xSequenceEnables_t *enables)
{
    uint8_t config;

    if (!ReadRegister(VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, &config))
    {
        return false;
    }
    enables->tcc = ((config >> 4U) & 1U) != 0U;
    enables->dss = ((config >> 3U) & 1U) != 0U;
    enables->msrc = ((config >> 2U) & 1U) != 0U;
    enables->preRange = ((config >> 6U) & 1U) != 0U;
    enables->finalRange = ((config >> 7U) & 1U) != 0U;
    return true;
}

static bool GetSequenceTimeouts(const Vl53l0xSequenceEnables_t *enables,
                                Vl53l0xSequenceTimeouts_t *timeouts)
{
    uint8_t value;
    uint16_t encoded;

    if (!ReadRegister(VL53L0X_REG_PRE_RANGE_VCSEL_PERIOD, &value))
    {
        return false;
    }
    timeouts->preRangeVcselPeriodPclks = DecodeVcselPeriod(value);

    if (!ReadRegister(VL53L0X_REG_MSRC_TIMEOUT, &value))
    {
        return false;
    }
    timeouts->msrcDssTccMclks = (uint16_t)value + 1U;
    timeouts->msrcDssTccUs =
        TimeoutMclksToUs(timeouts->msrcDssTccMclks,
                         timeouts->preRangeVcselPeriodPclks);

    if (!ReadRegister16(VL53L0X_REG_PRE_RANGE_TIMEOUT_HI, &encoded))
    {
        return false;
    }
    timeouts->preRangeMclks = DecodeTimeout(encoded);
    timeouts->preRangeUs =
        TimeoutMclksToUs(timeouts->preRangeMclks,
                         timeouts->preRangeVcselPeriodPclks);

    if (!ReadRegister(VL53L0X_REG_FINAL_RANGE_VCSEL_PERIOD, &value) ||
        !ReadRegister16(VL53L0X_REG_FINAL_RANGE_TIMEOUT_HI, &encoded))
    {
        return false;
    }
    timeouts->finalRangeVcselPeriodPclks = DecodeVcselPeriod(value);
    timeouts->finalRangeMclks = DecodeTimeout(encoded);
    if (enables->preRange && timeouts->finalRangeMclks > timeouts->preRangeMclks)
    {
        timeouts->finalRangeMclks -= timeouts->preRangeMclks;
    }
    timeouts->finalRangeUs =
        TimeoutMclksToUs(timeouts->finalRangeMclks,
                         timeouts->finalRangeVcselPeriodPclks);
    return true;
}

static bool GetMeasurementTimingBudget(uint32_t *budgetUs)
{
    const uint16_t startOverhead = 1910U;
    const uint16_t endOverhead = 960U;
    const uint16_t msrcOverhead = 660U;
    const uint16_t tccOverhead = 590U;
    const uint16_t dssOverhead = 690U;
    const uint16_t preRangeOverhead = 660U;
    const uint16_t finalRangeOverhead = 550U;
    Vl53l0xSequenceEnables_t enables;
    Vl53l0xSequenceTimeouts_t timeouts;
    uint32_t budget = (uint32_t)startOverhead + endOverhead;

    if (!GetSequenceEnables(&enables) || !GetSequenceTimeouts(&enables, &timeouts))
    {
        return false;
    }

    if (enables.tcc)
    {
        budget += timeouts.msrcDssTccUs + tccOverhead;
    }
    if (enables.dss)
    {
        budget += 2U * (timeouts.msrcDssTccUs + dssOverhead);
    }
    else if (enables.msrc)
    {
        budget += timeouts.msrcDssTccUs + msrcOverhead;
    }
    if (enables.preRange)
    {
        budget += timeouts.preRangeUs + preRangeOverhead;
    }
    if (enables.finalRange)
    {
        budget += timeouts.finalRangeUs + finalRangeOverhead;
    }
    *budgetUs = budget;
    return true;
}

static bool SetMeasurementTimingBudget(uint32_t budgetUs)
{
    const uint16_t startOverhead = 1910U;
    const uint16_t endOverhead = 960U;
    const uint16_t msrcOverhead = 660U;
    const uint16_t tccOverhead = 590U;
    const uint16_t dssOverhead = 690U;
    const uint16_t preRangeOverhead = 660U;
    const uint16_t finalRangeOverhead = 550U;
    Vl53l0xSequenceEnables_t enables;
    Vl53l0xSequenceTimeouts_t timeouts;
    uint32_t usedUs = (uint32_t)startOverhead + endOverhead;
    uint32_t finalRangeTimeoutMclks;

    if (!GetSequenceEnables(&enables) || !GetSequenceTimeouts(&enables, &timeouts))
    {
        return false;
    }

    if (enables.tcc)
    {
        usedUs += timeouts.msrcDssTccUs + tccOverhead;
    }
    if (enables.dss)
    {
        usedUs += 2U * (timeouts.msrcDssTccUs + dssOverhead);
    }
    else if (enables.msrc)
    {
        usedUs += timeouts.msrcDssTccUs + msrcOverhead;
    }
    if (enables.preRange)
    {
        usedUs += timeouts.preRangeUs + preRangeOverhead;
    }
    if (!enables.finalRange)
    {
        return true;
    }

    usedUs += finalRangeOverhead;
    if (usedUs > budgetUs)
    {
        return false;
    }

    finalRangeTimeoutMclks =
        TimeoutUsToMclks(budgetUs - usedUs,
                         timeouts.finalRangeVcselPeriodPclks);
    if (enables.preRange)
    {
        finalRangeTimeoutMclks += timeouts.preRangeMclks;
    }
    return WriteRegister16(VL53L0X_REG_FINAL_RANGE_TIMEOUT_HI,
                           EncodeTimeout(finalRangeTimeoutMclks));
}

static bool PerformSingleReferenceCalibration(uint8_t vhvInitByte)
{
    return WriteRegister(VL53L0X_REG_SYSRANGE_START,
                         (uint8_t)(0x01U | vhvInitByte)) &&
           WaitForNewSample() &&
           WriteRegister(VL53L0X_REG_INTERRUPT_CLEAR, VL53L0X_CLEAR_INTERRUPT) &&
           WriteRegister(VL53L0X_REG_SYSRANGE_START, 0x00U);
}

static bool StartContinuous(void)
{
    return WriteRegister(0x80U, 0x01U) &&
           WriteRegister(0xFFU, 0x01U) &&
           WriteRegister(0x00U, 0x00U) &&
           WriteRegister(0x91U, s_stopVariable) &&
           WriteRegister(0x00U, 0x01U) &&
           WriteRegister(0xFFU, 0x00U) &&
           WriteRegister(0x80U, 0x00U) &&
           WriteRegister(VL53L0X_REG_SYSRANGE_START, VL53L0X_START_CONTINUOUS);
}

bool Vl53l0xSensor_Init(void)
{
    uint8_t value;
    uint32_t timingBudgetUs;

    s_lastSampleTick = 0U;

    if (!ReadRegister(VL53L0X_REG_IDENTIFICATION_MODEL_ID, &value) ||
        value != VL53L0X_EXPECTED_MODEL_ID)
    {
        return false;
    }

    /*
     * The QUT I2C breakout supplies 3.3 V. Select the sensor's high-voltage
     * I/O pad mode before doing its one-time boot setup.
     */
    if (!ReadRegister(VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA, &value) ||
        !WriteRegister(VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA,
                       (uint8_t)(value | 0x01U)) ||
        !WriteRegister(0x88U, 0x00U) ||
        !WriteRegister(0x80U, 0x01U) ||
        !WriteRegister(0xFFU, 0x01U) ||
        !WriteRegister(0x00U, 0x00U) ||
        !ReadRegister(0x91U, &s_stopVariable) ||
        !WriteRegister(0x00U, 0x01U) ||
        !WriteRegister(0xFFU, 0x00U) ||
        !WriteRegister(0x80U, 0x00U) ||
        !ReadRegister(VL53L0X_REG_MSRC_CONFIG_CONTROL, &value) ||
        !WriteRegister(VL53L0X_REG_MSRC_CONFIG_CONTROL,
                       (uint8_t)(value | 0x12U)) ||
        !WriteRegister16(VL53L0X_REG_FINAL_RANGE_MIN_COUNT_RATE, 0x0020U) ||
        !WriteRegister(VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0xFFU) ||
        !ConfigureReferenceSpads() ||
        !WriteSettings(s_defaultTuningSettings,
                       (uint32_t)(sizeof(s_defaultTuningSettings) /
                                  sizeof(s_defaultTuningSettings[0]))) ||
        !WriteRegister(VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04U) ||
        !ReadRegister(VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH, &value) ||
        !WriteRegister(VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH,
                       (uint8_t)(value & (uint8_t)~0x10U)) ||
        !WriteRegister(VL53L0X_REG_INTERRUPT_CLEAR, VL53L0X_CLEAR_INTERRUPT) ||
        !GetMeasurementTimingBudget(&timingBudgetUs) ||
        !WriteRegister(VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0xE8U) ||
        !SetMeasurementTimingBudget(timingBudgetUs) ||
        !WriteRegister(VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0x01U) ||
        !PerformSingleReferenceCalibration(0x40U) ||
        !WriteRegister(VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0x02U) ||
        !PerformSingleReferenceCalibration(0x00U) ||
        !WriteRegister(VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0xE8U))
    {
        return false;
    }

    return StartContinuous();
}

void Vl53l0xSensor_Sample(void)
{
    uint8_t status = 0U;
    uint8_t rxData[2] = {0U, 0U};
    uint16_t distanceMm;
    TickType_t currentTick = xTaskGetTickCount();

    if ((currentTick - s_lastSampleTick) < pdMS_TO_TICKS(VL53L0X_SAMPLE_PERIOD_MS))
    {
        return;
    }
    s_lastSampleTick = currentTick;

    if (!ReadRegister(VL53L0X_REG_RESULT_INTERRUPT, &status) ||
        (status & VL53L0X_NEW_SAMPLE_MASK) == 0U ||
        !readI2C_VL53L0X(VL53L0X_I2C_ADDRESS,
                          VL53L0X_REG_RESULT_RANGE_MM,
                          rxData,
                          2U))
    {
        return;
    }

    distanceMm = ((uint16_t)rxData[0] << 8) | rxData[1];
    if (distanceMm == 0U)
    {
        distanceMm = VL53L0X_CLOSE_DISTANCE_MM;
    }
    else if (distanceMm >= VL53L0X_INVALID_DISTANCE_MM)
    {
        /* No target is a real clear reading; queue it so E-stop can release. */
        distanceMm = VL53L0X_INVALID_DISTANCE_MM;
    }

    if (rawdataQueue != NULL)
    {
        SensorMsg_t msg = {
            .type = SENSOR_MSG_DISTANCE,
            .value = (uint32_t)distanceMm
        };
        (void)xQueueSendToBack(rawdataQueue, &msg, 0U);
    }
    (void)WriteRegister(VL53L0X_REG_INTERRUPT_CLEAR, VL53L0X_CLEAR_INTERRUPT);
}
