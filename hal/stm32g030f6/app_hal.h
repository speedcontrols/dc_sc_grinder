#ifndef __APP_HAL__
#define __APP_HAL__

#include <stdint.h>
#include "libfixmath/fix16.h"
#include "stm32g0xx_hal.h"

#define SAMPLING_RATE 16384

// Oversampling ratio. Used to define buffer sizes
#define ADC_FETCH_PER_TICK 1

// How many channels are sampled "in parallel".
// Used to define global DMA buffer size.
#define ADC_CHANNELS_COUNT 2


#define GET_TIMESTAMP() HAL_GetTick()


namespace hal {

void setup();
void set_power(fix16_t duty_cycle);

} // namespace

#endif