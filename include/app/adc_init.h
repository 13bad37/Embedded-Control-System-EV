#define POWER_ADC_PERIPH_A          SYSCTL_PERIPH_GPIOE
#define POWER_ADC_PORT_A            GPIO_PORTE_BASE
#define POWER_ADC_PIN_A             GPIO_PIN_3      // ISENC -> AIN0

#define POWER_ADC_PERIPH_B          SYSCTL_PERIPH_GPIOD
#define POWER_ADC_PORT_B            GPIO_PORTD_BASE
#define POWER_ADC_PIN_B             GPIO_PIN_7      // ISENB -> AIN4

#define POWER_ADC_BASE              ADC0_BASE
#define POWER_ADC_SEQUENCER         2               // SS2: 4 samples, good for 2 channels
#define POWER_ADC_CH_A              ADC_CTL_CH0     // AIN0
#define POWER_ADC_CH_B              ADC_CTL_CH4     // AIN4

void prvADCInit(void);