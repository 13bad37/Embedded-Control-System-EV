/**************************************************************************************************
*  Filename:       i2cOptDriver.h
*  By:             Jesse Haviland
*  Created:        1 February 2019
*  Revised:        2 February 2019
*  Revision:       1.0
*
*  Description:    i2c Driver for use with the Texas Instruments OP3001 Optical Sensor
*************************************************************************************************/



#ifndef _I2COPTDRIVER_H_
#define _I2COPTDRIVER_H_



// ----------------------- Includes -----------------------
#include <stdbool.h>
#include <stdint.h>


    
// ----------------------- Exported prototypes -----------------------
extern bool writeI2C2_SHT31(uint8_t ui8Addr, uint8_t cmdMSB, uint8_t cmdLSB);
extern bool readI2C2_SHT31(uint8_t ui8Addr, uint8_t *data, uint16_t numBytes);

extern bool readI2C_VL53L0X(uint8_t ui8Addr, uint8_t ui8Reg, uint8_t *data, uint16_t numBytes);
extern bool writeI2C_VL53L0X_Byte(uint8_t ui8Addr, uint8_t ui8Reg, uint8_t data);



#endif /* _I2COPTDRIVER_H_ */
