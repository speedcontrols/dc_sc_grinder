#include "meter.h"
#include "config.h"
#include "eeprom.h"


void Meter::configure()
{
    magnitude2_treshold = eeprom_uint32_read(
        CFG_METER_MAGNITUDE_NOISE_TRESHOLD_ADDR,
        0
    );
}


void Meter::reset_state()
{
    collected = 0;
}


bool Meter::consume(io_data_t &io_data)
{
    // Collect data for FFT

    if (collected < FFT_SIZE) {
        fft_buf[collected++] = { .r = io_data.current, .i = 0 };
        return false;
    }

    collected = 0;

    // Do FFT and search peak.
    fft_fft(fft_buf, FFT_SIZE_BITS);

    uint32_t max = 0;
    uint32_t max_idx = 0;

    for (uint16_t i = FFT_SKIP_POINTS; i < FFT_SIZE/2; i++)
    {
        uint32_t acc0 = (uint32_t) (((int64_t)fft_buf[i].r * fft_buf[i].r ) >> 33);
        uint32_t acc1 = (uint32_t) (((int64_t)fft_buf[i].i * fft_buf[i].i ) >> 33);
        uint32_t magn2 = acc0 + acc1;

        if (magn2 > max) { max = magn2; max_idx = 0; }
    }

    if (max < magnitude2_treshold)
    {
        frequency = 0;
        rpm = 0;
        magnitude2 = 0;
        return true;
    }

    frequency = (max_idx * SAMPLING_RATE + FFT_SIZE / 2) / FFT_SIZE;
    magnitude2 = max;

    return true;
}
