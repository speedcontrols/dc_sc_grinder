#include "regulator.h"
#include "app_hal.h"
#include "eeprom.h"

void Regulator::disable()
{
    enabled = false;
}

void Regulator::enable()
{
    enabled = true;
    adrc_freq_estimated = 0;
    adrc_correction = 0;
}

// Calculate internal observers parameters L1, L2
// based on current cfg_adrc_Kobservers value
void Regulator::adrc_update_observers_parameters()
{
    fix16_t mul_Ko_Kp = fix16_mul(cfg_adrc_Kobservers, cfg_adrc_Kp);
    adrc_L1 = 2 * mul_Ko_Kp;
    adrc_L2 = fix16_mul(mul_Ko_Kp, mul_Ko_Kp);
}

// Convert knob value to normalized frequency setpoint in fix16_t format.
// Returns:
// - zero in dead zone
// - [ min_freq/max_freq ... 1.0 ]
void Regulator::apply_knob(fix16_t knob)
{
    if (knob < F16(KNOB_DEAD_ZONE_WIDTH)) {
        setpoint = 0;
        return;
    };

    setpoint = fix16_mul(
        (knob - F16(KNOB_DEAD_ZONE_WIDTH)),
        knob_norm_coeff
    ) + cfg_freq_min_limit_norm;

    return;
}

void Regulator::tick()
{
    if (!enabled) return;

    // Normalize frequency to [0.0 ... 1.0]
    fix16_t freq_norm = fix16_mul(fix16_from_int(freq_in), freq_norm_coeff);

    // 1-st order ADRC by https://arxiv.org/pdf/1908.04596.pdf (augmented)

    // Proportional correction signal,
    // makes reaction to motor load change
    // significantly faster
    fix16_t adrc_p_correction = fix16_mul((freq_norm - adrc_freq_estimated), cfg_adrc_p_corr_coeff);

    // u0 - output of linear proportional controller in ADRC system
    fix16_t u0 = fix16_mul((setpoint - adrc_freq_estimated), cfg_adrc_Kp);
    // 2 state observers:
    //   - speed observer (adrc_freq_estimated)
    //   - generalized disturbance observer (adrc_correction)
    adrc_correction += fix16_mul(
        fix16_mul(freq_norm - adrc_freq_estimated, adrc_L2),
        integr_coeff
    );
    adrc_freq_estimated += fix16_mul(
        u0 + fix16_mul(adrc_L1, (freq_norm - adrc_freq_estimated)),
        integr_coeff
    );

    // Clamp value
    if (adrc_freq_estimated < cfg_freq_min_limit_norm) adrc_freq_estimated = cfg_freq_min_limit_norm;
    if (adrc_freq_estimated > cfg_freq_max_limit_norm) adrc_freq_estimated = cfg_freq_max_limit_norm;

    fix16_t output = fix16_mul(
        (u0 - adrc_correction - adrc_p_correction),
        adrc_b0_inv
    );

    // Anti-Windup
    // 0 <= output <= 1
    //
    // output = (u0 - adrc_correction)/b0,
    // so if output = cfg_freq_min_limit_norm -> adrc_correction = u0 - b0 * cfg_freq_min_limit_norm
    if (output < cfg_freq_min_limit_norm)
    {
        output = cfg_freq_min_limit_norm;
        adrc_correction = u0 - fix16_div(cfg_freq_min_limit_norm, adrc_b0_inv);
    }

    // output = (u0 - adrc_correction)/b0,
    // so if output = cfg_freq_max_limit_norm -> adrc_correction = u0 - b0 * cfg_freq_max_limit_norm
    if (output > cfg_freq_max_limit_norm)
    {
        output = cfg_freq_max_limit_norm;
        adrc_correction = u0 - fix16_div(cfg_freq_max_limit_norm, adrc_b0_inv);
    }

    power_out = output;
    hal::set_power(output);
}

void Regulator::configure()
{
    float _rpm_max = eeprom_float_read(CFG_RPM_MAX_ADDR, CFG_RPM_MAX_DEFAULT);

    freq_norm_coeff = fix16_from_float(1.0f / (_rpm_max * MOTOR_POLES / 60));

    cfg_freq_max_limit_norm = F16(0.8);

    if (_rpm_max > MOTOR_MAX_RPM_LIMIT) {
        cfg_freq_max_limit_norm = fix16_from_float(MOTOR_MAX_RPM_LIMIT / _rpm_max);
    }

    cfg_freq_min_limit_norm = fix16_from_float(MOTOR_MIN_RPM_LIMIT / _rpm_max);

    knob_norm_coeff =  fix16_div(
        cfg_freq_max_limit_norm - cfg_freq_min_limit_norm,
        fix16_one - F16(KNOB_DEAD_ZONE_WIDTH)
    );

    cfg_adrc_Kp = fix16_from_float(
        eeprom_float_read(CFG_ADRC_KP_ADDR, CFG_ADRC_KP_DEFAULT)
    );
    cfg_adrc_Kobservers = fix16_from_float(
        eeprom_float_read(CFG_ADRC_KOBSERVERS_ADDR, CFG_ADRC_KOBSERVERS_DEFAULT)
    );
    cfg_adrc_p_corr_coeff = fix16_from_float(
        eeprom_float_read(CFG_ADRC_P_CORR_COEFF_ADDR, CFG_ADRC_P_CORR_COEFF_DEFAULT)
    );

    adrc_b0_inv = F16(1.0f / ADRC_BO);

    adrc_update_observers_parameters();

    min_power_treshold = eeprom_uint32_read(CFG_MIN_POWER_TRESHOLD_ADDR, 0);
}
