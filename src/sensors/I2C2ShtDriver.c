#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"



typedef enum { 
    IDLE_1,
    WRITED0,
    WRITED1,
    WRITEREAD,
    READD0,
    READD1,
    SHT_WRITE_CMD,
    SHT_READ_DATA,
    VL_WRITE_DATA,  
    VL_READ_START,   
    VL_READ_FINISH
} i2c2_state;

volatile uint8_t I2C2Address;
volatile i2c2_state state_2 = IDLE_1;
volatile uint8_t* I2Cdata_2;
volatile uint16_t I2CbytesCount_2;
volatile uint16_t I2CbytesTotal_2;
volatile bool i2c2_success = false;

extern SemaphoreHandle_t I2C2Semaphore;
void I2C2_Handler(void)
{
    BaseType_t xSensorTaskWoken = pdFALSE;

    I2CMasterIntClear(I2C2_BASE);

    if (I2CMasterErr(I2C2_BASE) != I2C_MASTER_ERR_NONE) 
    { 
        state_2 = IDLE_1;
        i2c2_success = false; 
        xSemaphoreGiveFromISR(I2C2Semaphore, &xSensorTaskWoken);
        portYIELD_FROM_ISR(xSensorTaskWoken);
        return; 
    }

    switch (state_2)
    {
        //======================== OPT3001 States =====================
        case WRITED0:
            I2CMasterDataPut(I2C2_BASE, I2Cdata_2[0]);
            I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
            state_2 = WRITED1;
            break;
        case WRITED1:
            I2CMasterDataPut(I2C2_BASE, I2Cdata_2[1]);
            I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
            state_2 = IDLE_1;
            xSemaphoreGiveFromISR(I2C2Semaphore, &xSensorTaskWoken);
            break;
        case WRITEREAD:
            I2CMasterSlaveAddrSet(I2C2_BASE, I2C2Address, true);
            I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);
            state_2 = READD0;
            break;
        case READD0:
            I2Cdata_2[0] = I2CMasterDataGet(I2C2_BASE);
            I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
            state_2 = READD1;
            break;
        case READD1:
            I2Cdata_2[1] = I2CMasterDataGet(I2C2_BASE);
            state_2 = IDLE_1;
            xSemaphoreGiveFromISR(I2C2Semaphore, &xSensorTaskWoken);
            break;
        // ======================= SHT31 States =======================
        case SHT_WRITE_CMD:
            I2CMasterDataPut(I2C2_BASE, I2Cdata_2[0]); 
            I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
            state_2 = IDLE_1;
            xSemaphoreGiveFromISR(I2C2Semaphore, &xSensorTaskWoken);
            break;

        case SHT_READ_DATA:
            // first byte
            I2Cdata_2[I2CbytesTotal_2] = I2CMasterDataGet(I2C2_BASE);
            I2CbytesTotal_2++;

            if (I2CbytesTotal_2 < (I2CbytesCount_2 - 1)) {
                // continue
                I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
            } 
            else if (I2CbytesTotal_2 == (I2CbytesCount_2 - 1)) {
                // second last
                I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
            } 
            else {

                state_2 = IDLE_1;
                xSemaphoreGiveFromISR(I2C2Semaphore, &xSensorTaskWoken);
            }
            break;
            
        // ======================= VL53L0x States =======================
        case VL_WRITE_DATA:
            // Send the single byte of data and release the bus
            I2CMasterDataPut(I2C2_BASE, I2Cdata_2[0]);
            I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
            state_2 = IDLE_1;
            xSemaphoreGiveFromISR(I2C2Semaphore, &xSensorTaskWoken);
            break;

        case VL_READ_START:
            // Flip to read mode and request exactly 1 byte
            I2CMasterSlaveAddrSet(I2C2_BASE, I2C2Address, true); 
            
            if (I2CbytesTotal_2 == 1) {
                I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
            } else {
                I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);
            }
            state_2 = VL_READ_FINISH;
            break;

        case VL_READ_FINISH:
            I2Cdata_2[I2CbytesCount_2] = I2CMasterDataGet(I2C2_BASE);
            I2CbytesCount_2++;

            if (I2CbytesCount_2 == I2CbytesTotal_2) {
                //last
                state_2 = IDLE_1;
                xSemaphoreGiveFromISR(I2C2Semaphore, &xSensorTaskWoken);
            } 
            else if (I2CbytesCount_2 == (I2CbytesTotal_2 - 1)) {
                // second last
                I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
            } 
            else {
                // Keep reading 
                I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
            }
            break;
        default:
            state_2 = IDLE_1;
            break;
    }

    portYIELD_FROM_ISR(xSensorTaskWoken);
}

// ======================= OPT3001 Read and Write Functions =======================
bool writeI2C(uint8_t ui8Addr, uint8_t ui8Reg, uint8_t *data)
{   
    I2Cdata_2 = data;
    state_2 = WRITED0;

    // Load device slave address
    I2CMasterSlaveAddrSet(I2C2_BASE, ui8Addr, false);

    // Place the character to be sent in the data register
    I2CMasterDataPut(I2C2_BASE, ui8Reg);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    if (xSemaphoreTake(I2C2Semaphore, pdMS_TO_TICKS(100)) == pdTRUE) 
    {
        return true;
    }
    state_2 = IDLE_1;
    return false;
}



/*
 * Sets slave address to ui8Addr
 * Writes ui8Reg over i2c to specify register being read from
 * Reads two bytes from i2c slave and stores them into *data
 */
bool readI2C(uint8_t ui8Addr, uint8_t ui8Reg, uint8_t *data)
{
    I2Cdata_2 = data;
    I2C2Address = ui8Addr;
    state_2 = WRITEREAD;


    // Load device slave address and change I2C to write
    I2CMasterSlaveAddrSet(I2C2_BASE, ui8Addr, false);

    // Place the character to be sent in the data register
    I2CMasterDataPut(I2C2_BASE, ui8Reg);

    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    //I2CMasterSlaveAddrSet(I2C0_BASE, ui8Addr, true);
    if (xSemaphoreTake(I2C2Semaphore, pdMS_TO_TICKS(100)) == pdTRUE) 
    {
        return true;
    }
    state_2 = IDLE_1;
    return false;
}



// ======================= SHT31 read and write functions =======================
bool writeI2C2_SHT31(uint8_t ui8Addr, uint8_t cmdMSB, uint8_t cmdLSB)
{
    static uint8_t lsbBuffer;
    lsbBuffer = cmdLSB;
    
    I2Cdata_2 = &lsbBuffer;
    state_2 = SHT_WRITE_CMD;
    i2c2_success = true;

    // Set slave address for writing
    I2CMasterSlaveAddrSet(I2C2_BASE, ui8Addr, false);
    
    // Put MSB in buffer and start the burst
    I2CMasterDataPut(I2C2_BASE, cmdMSB);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_START);


    if (xSemaphoreTake(I2C2Semaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        return i2c2_success;
    }
    
    state_2 = IDLE_1;
    return false;
}

bool readI2C2_SHT31(uint8_t ui8Addr, uint8_t *data, uint16_t numBytes)
{
    I2Cdata_2 = data;
    I2CbytesCount_2 = numBytes; 
    I2CbytesTotal_2 = 0;
    state_2 = SHT_READ_DATA;
    i2c2_success = true;


    I2CMasterSlaveAddrSet(I2C2_BASE, ui8Addr, true); 
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);

    if (xSemaphoreTake(I2C2Semaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        return i2c2_success;
    }
    
    state_2 = IDLE_1;
    return false;
}

// ======================= VL53L0x Read and Write Functions =======================
bool writeI2C_VL53L0X_Byte(uint8_t ui8Addr, uint8_t ui8Reg, uint8_t data)
{
    static uint8_t txData;
    txData = data;
    I2Cdata_2 = &txData;
    state_2 = VL_WRITE_DATA;
    i2c2_success = true;

    I2CMasterSlaveAddrSet(I2C2_BASE, ui8Addr, false);
    I2CMasterDataPut(I2C2_BASE, ui8Reg);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    if (xSemaphoreTake(I2C2Semaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        return i2c2_success;
    }
    state_2 = IDLE_1;
    return false;
}

// Handles both 1-byte Status reads and 2-byte Distance reads!
bool readI2C_VL53L0X(uint8_t ui8Addr, uint8_t ui8Reg, uint8_t *data, uint16_t numBytes)
{
    I2Cdata_2 = data;
    I2C2Address = ui8Addr;
    I2CbytesTotal_2 = numBytes; 
    I2CbytesCount_2 = 0;
    state_2 = VL_READ_START;
    i2c2_success = true;


    I2CMasterSlaveAddrSet(I2C2_BASE, ui8Addr, false);
    I2CMasterDataPut(I2C2_BASE, ui8Reg);
    I2CMasterControl(I2C2_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    if (xSemaphoreTake(I2C2Semaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        return i2c2_success;
    }
    state_2 = IDLE_1;
    return false;
}