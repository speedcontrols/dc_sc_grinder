#ifndef __IO__
#define __IO__

#include "etl/queue_spsc_atomic.h"
#include "libfixmath/fix16.h"

struct io_data_t {
    uint16_t current = 0;
};


class Io
{
public:
    // Output data to process in main loop. In theory should have 1 element max.
    // Leave room for 3 more for sure.
    etl::queue_spsc_atomic<io_data_t, 10, etl::memory_model::MEMORY_MODEL_SMALL> out;

    // Calculated knob value
    fix16_t knob = 0;

    // Eat raw adc data from interrupt, and:
    // - produce knob value
    // - fire current to queue (for postponed processing)
    void consume(uint16_t adc_current_buf[], uint16_t adc_knob_buf[]);

private:
    // Previous iteration values
    fix16_t prev_knob = 0;
};


#endif
