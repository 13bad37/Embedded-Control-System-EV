#include <stdint.h>
#include <stdbool.h>


bool AmbientLightSensor_Init(void);

void AmbientLightSensor_Sample(void);

bool AmbientLightSensor_GetRawLux(int32_t *lux);

bool AmbientLightSensor_GetFilteredLux(int32_t *lux);
