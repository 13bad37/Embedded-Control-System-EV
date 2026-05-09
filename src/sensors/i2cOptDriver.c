/**************************************************************************************************
*  Filename:       i2cOptDriver.c
*  By:             Jesse Haviland
*  Created:        1 February 2019
*  Revised:        23 March 2019
*  Revision:       2.0
*
*  Description:    i2c Driver for use with opt3001.c and the TI OP3001 Optical Sensor
*************************************************************************************************/

// ----------------------- Includes -----------------------
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "sensors/i2cOptDriver.h"
#include "inc/hw_memmap.h"
#include "tivaware_driverlib/i2c.h"
#include "utils/uartstdio.h"
#include "driverlib/sysctl.h"


extern SemaphoreHandle_t I2C0Semaphore;

typedef enum { //make switch case for I2C ISR so it will never be in a waiting state and will immediately write or read datas
    IDLE,
    WRITED0,
    WRITED1,
    WRITEREAD,
    READD0,
    READD1
} i2c_state;

volatile i2c_state state = IDLE;
volatile uint8_t I2CAddress;
volatile uint8_t* I2Cdata;
volatile uint16_t I2CbytesTotal;
volatile uint16_t I2CbytesCount;



void I2C0_Handler(void)
{
    BaseType_t xSensorTaskWoken;
    

    xSensorTaskWoken = pdFALSE;
    
    I2CMasterIntClear(I2C0_BASE);

    if (I2CMasterErr(I2C0_BASE) != I2C_MASTER_ERR_NONE) //error handling - if error return to task without giving semaphore, otherwise will wait for data that isn't there
    { 
        state = IDLE;
        xSemaphoreGiveFromISR(I2C0Semaphore, &xSensorTaskWoken);
        portYIELD_FROM_ISR(xSensorTaskWoken);
        return; 
    }

    switch (state)
    {
        case WRITED0:
            I2CMasterDataPut(I2C0_BASE, I2Cdata[0]);
            I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
            state = WRITED1;
            break;
        case WRITED1:
            I2CMasterDataPut(I2C0_BASE, I2Cdata[1]);
            I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
            state = IDLE;
            xSemaphoreGiveFromISR(I2C0Semaphore, &xSensorTaskWoken);
            break;
        case WRITEREAD:
            I2CMasterSlaveAddrSet(I2C0_BASE, I2CAddress, true);
            I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);
            state = READD0;
            break;
        case READD0:
            I2Cdata[0] = I2CMasterDataGet(I2C0_BASE);
            I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
            state = READD1;
            break;
        case READD1:
            I2Cdata[1] = I2CMasterDataGet(I2C0_BASE);
            state = IDLE;
            xSemaphoreGiveFromISR(I2C0Semaphore, &xSensorTaskWoken);
            break;
    }

    portYIELD_FROM_ISR( xSensorTaskWoken );
}

/*
 * Sets slave address to ui8Addr
 * Puts ui8Reg followed by two data bytes in *data and transfers
 * over i2c
 */
bool writeI2C(uint8_t ui8Addr, uint8_t ui8Reg, uint8_t *data)
{   
    I2Cdata = data;
    state = WRITED0;

    // Load device slave address
    I2CMasterSlaveAddrSet(I2C0_BASE, ui8Addr, false);

    // Place the character to be sent in the data register
    I2CMasterDataPut(I2C0_BASE, ui8Reg);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    if (xSemaphoreTake(I2C0Semaphore, pdMS_TO_TICKS(100)) == pdTRUE) 
    {
        return true;
    }
    state = IDLE;
    return false;
}



/*
 * Sets slave address to ui8Addr
 * Writes ui8Reg over i2c to specify register being read from
 * Reads two bytes from i2c slave and stores them into *data
 */
bool readI2C(uint8_t ui8Addr, uint8_t ui8Reg, uint8_t *data)
{
    I2Cdata = data;
    I2CAddress = ui8Addr;
    state = WRITEREAD;


    // Load device slave address and change I2C to write
    I2CMasterSlaveAddrSet(I2C0_BASE, ui8Addr, false);

    // Place the character to be sent in the data register
    I2CMasterDataPut(I2C0_BASE, ui8Reg);

    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    //I2CMasterSlaveAddrSet(I2C0_BASE, ui8Addr, true);
    if (xSemaphoreTake(I2C0Semaphore, pdMS_TO_TICKS(100)) == pdTRUE) 
    {
        return true;
    }
    state = IDLE;
    return false;
}



