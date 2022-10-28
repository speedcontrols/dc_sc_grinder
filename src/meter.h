#ifndef __METER__
#define __METER__


#include "app_hal.h"
#include "io.h"
#include "fft.h"


#define FFT_SIZE 512
#define FFT_SIZE_BITS 9

// We should filter 100/120Hz + 2/3/4 harmonics
#define FFT_TRESHOLD_FREQUENCY 500

// Number of points to ignore from the start
#define FFT_SKIP_POINTS (FFT_TRESHOLD_FREQUENCY * FFT_SIZE / SAMPLING_RATE + 1)

class Meter
{
public:

    // Detected frequency & RPM
    uint32_t frequency = 0;
    uint32_t rpm = 0;

    // Detected energy^2 (for noise treshold)
    uint32_t magnitude2 = 0;

    // Noise treshold, if below => force speed = 0
    uint32_t magnitude2_treshold = 0;

    void configure();
    bool consume(io_data_t &io_data);
    void reset_state();

private:
    uint16_t collected = 0;

    fft_complex_t fft_buf[FFT_SIZE];
};


#endif
