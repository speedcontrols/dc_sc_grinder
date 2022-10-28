#ifndef __REGULATOR__
#define __REGULATOR__

#include "libfixmath/fix16.h"
#include "config.h"

// ADRC iteration frequency, Hz. To fit math in fix16 without overflow.
// Observers in ADRC system must have performance much higher
// than motor performance, so set iteration frequency to 1000 Hz
#define APP_ADRC_FREQUENCY 40

// b0 = K/T, where K=1 due to speed and triac setpoint normalization,
// T - motor time constant, estimated by calibration (see calibrator_adrc.h)
#define ADRC_BO 5.0f

// Coefficient used by ADRC observers integrators
constexpr fix16_t integr_coeff = F16(1.0 / APP_ADRC_FREQUENCY);

class Regulator
{
public:
    // Measured frequency.
    // We use frequency instead of RPM, because it better fits into fix16_t.
    uint32_t freq_in = 0;

    // For callibrator only. Normalized frequency setpoint for direct control
    // from calibrator. Updated by apply_knob() in normal case.
    fix16_t setpoint = 0;

    // For calibrator only. Copy of output power, to calculate minimal treshold.
    fix16_t power_out = 0;

    // ADRC coefficients
    fix16_t cfg_adrc_Kp;
    fix16_t cfg_adrc_Kobservers;
    fix16_t cfg_adrc_p_corr_coeff;

    fix16_t adrc_b0_inv;

    // Power should not go below some limit for 2 reasons:
    // - 10% required for correct ADC work.
    // - 20% required to reach detectable speed.
    // Calibrator set minimal speed and remember power value as minimal treshold
    fix16_t min_power_treshold = 0;

    void disable();
    void enable();
    void tick();
    void configure();
    void apply_knob(fix16_t knob);
    void adrc_update_observers_parameters();

private:
    bool enabled = false;

    // Config limits are now in normalized [0.0..1.0] form of max motor frequency.
    fix16_t cfg_freq_max_limit_norm;
    fix16_t cfg_freq_min_limit_norm;

    // Frequency normalization scale, convert to range [0.0 ... 1.0]
    // Calculated on config load
    fix16_t freq_norm_coeff = F16(1.0f / MOTOR_MAX_RPM_LIMIT);

    // Cache for knob normalization, calculated on config load
    fix16_t knob_norm_coeff = F16(1);

    fix16_t adrc_correction;
    fix16_t adrc_freq_estimated;

    fix16_t adrc_L1;
    fix16_t adrc_L2;

    fix16_t knob_to_setpoint(fix16_t knob);
};

#endif
