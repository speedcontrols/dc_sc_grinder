#ifndef __CALIBRATOR_H__
#define __CALIBRATOR_H__

#include <stdint.h>
#include "stability_filter.h"

// Detect when user dials knob 3 times, start calibration sequence and
// update configuration.

class Calibrator
{
public:
    bool done = false;
    bool active = false;

    void configure();
    bool tick();

private:
    uint32_t dials_cnt;
    bool wait_knob_dial();

    uint32_t ts;
    uint64_t uint64_acc;
    fix16_t fix16_acc;

    fix16_t freq_max_speed;
    fix16_t freq_low_speed_point;
    fix16_t freq_high_speed_point;

    uint32_t start_time_ms;
    uint32_t stop_time_ms;
    uint32_t motor_start_stop_time;

    StabilityFilterTemplate<F16(2.0), 10> speed_tracker;

    uint32_t iterations_count;
    fix16_t iteration_step;

    fix16_t measure_amplitude_max_speed;
    fix16_t measure_amplitude_min_speed;

    // Holds value calculated during first attempt
    fix16_t first_iteration_amplitude;

    fix16_t adrc_param_attempt_value;

    fix16_t adrc_observers_calibrated_value;
    fix16_t adrc_kp_calibrated_value;
    fix16_t adrc_p_corr_coeff_calibrated_value;

    bool calibrate_adrc();
};

#endif
