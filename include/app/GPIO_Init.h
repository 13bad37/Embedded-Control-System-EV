#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"



#define HALL_A_PERIPH       SYSCTL_PERIPH_GPIOM
#define HALL_A_BASE         GPIO_PORTM_BASE
#define HALL_A_PIN          GPIO_PIN_3

#define HALL_B_PERIPH       SYSCTL_PERIPH_GPIOH
#define HALL_B_BASE         GPIO_PORTH_BASE
#define HALL_B_PIN          GPIO_PIN_2

#define HALL_C_PERIPH       SYSCTL_PERIPH_GPION
#define HALL_C_BASE         GPIO_PORTN_BASE
#define HALL_C_PIN          GPIO_PIN_2

void HallGpioInit(void);