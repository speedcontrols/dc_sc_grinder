#include "io.h"

void Io::consume(uint16_t adc_current_buf[], uint16_t adc_knob_buf[])
{
    // Simple smooth filter for knob
    uint16_t new_knob = uint16_t((prev_knob * 15 + adc_knob_buf[0]) >> 4);
    prev_knob = new_knob;

    knob = new_knob << 4;

    io_data_t io_data;
    io_data.current = adc_current_buf[0];

    // Push data to queue & drop queue content on overflow
    out.push(io_data);
}
