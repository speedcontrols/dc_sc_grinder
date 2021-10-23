#include "calibrator.h"
#include "../yield.h"
#include "../app.h"
#include "../eeprom.h"
#include "app_hal.h"

// Points to measure start/stop time between (fraction of max)
#define LOW_SPEED_POINT 0.3
#define HIGH_SPEED_POINT 0.7

// Minimal adrc_p_corr_coeff is 0 (proportional correction disabled)
#define MIN_ADRC_P_CORR_COEFF 0.0
// Minimal reasonable adrc_Kp * b0 value
#define MIN_ADRC_KPdivB0 0.3
// Minimal adrc_Kobservers value (observers disabled)
#define MIN_ADRC_KOBSERVERS 0.0
// Safe adrc_Kobservers value for adrc_Kp calibration
#define SAFE_ADRC_KOBSERVERS 1.0

#define INIT_KP_ITERATION_STEP 4.0
#define INIT_OBSERVERS_ITERATION_STEP 4.0
#define INIT_P_CORR_COEFF_ITERATION_STEP 5.0

// Desireable accuracy of ADRC calibration is 0.1
// We need 7 iterations to achieve this accuracy
// because (10 - 1)/2^7 = 0.07 < 0.1
#define MAX_ITERATIONS 7

// Maximum speed oscillation amplitude
// and speed overshoot values
// for ADRC_KP and ADRC_KOBSERVERS adjustment
#define MAX_AMPLITUDE 3.0
// Maximum speed oscillation amplitude
// and speed overshoot values
// for ADRC_P_CORR_COEFF adjustment
#define MAX_P_CORR_COEFF_AMPLITUDE 3.0

// Scale down ADRC coefficients to this value for safety
#define ADRC_SAFETY_SCALE 0.6
#define ADRC_P_CORR_COEFF_SAFETY_SCALE 0.6


bool Calibrator::calibrate_adrc()
{
    YIELDABLE;

    //
    // Measure max possible speed to count scale
    //

    hal::set_power(fix16_one);
    // Skip 1 sec to pass blind range [0..500Hz]
    YIELD_MS(1000);

    speed_tracker.reset();
    while (speed_tracker.is_stable())
    {
        YIELD_MS(100);
        speed_tracker.push(fix16_from_int(meter.frequency));
    }

    freq_max_speed = speed_tracker.average();

    // Save max rpm & reconfigure regulator
    eeprom_float_write(
        CFG_RPM_MAX_ADDR,
        fix16_to_float(fix16_mul(freq_max_speed, F16(60.0 / MOTOR_POLES)))
    );
    regulator.configure();

    freq_low_speed_point = fix16_mul(freq_max_speed, F16(LOW_SPEED_POINT));
    freq_high_speed_point = fix16_mul(freq_max_speed, F16(HIGH_SPEED_POINT));

    //
    // Measure slow down time
    //

    hal::set_power(F16(0.1));
    // wait until speed fall to high point
    YIELD_WHILE(fix16_from_int(meter.frequency) > freq_high_speed_point);

    YIELD_WHILE(fix16_from_int(meter.frequency) > freq_low_speed_point);
    stop_time_ms = YIELD_GET_MS();

    //
    // Measure speed up time
    //
    hal::set_power(fix16_one);
    YIELD_WHILE(fix16_from_int(meter.frequency) < freq_high_speed_point);
    start_time_ms = YIELD_GET_MS();

    // TODO: clarify
    motor_start_stop_time = (stop_time_ms + start_time_ms) * 2;


    //
    // Set speed for picking ADRC parameters.
    // Use lowest possible value, to reach stability if full operating range.
    //

    regulator.setpoint = fix16_div(
        F16(MOTOR_MIN_RPM_LIMIT * MOTOR_POLES / 60),
        fix16_from_int(freq_max_speed)
    );

    // Enable ADRC operation
    regulator.enable();

    //--------------------------------------------------------------------------
    // Pick ADRC_KP coeff value by half cut method
    //--------------------------------------------------------------------------

    iterations_count = 0;

    adrc_param_attempt_value = fix16_div(F16(MIN_ADRC_KPdivB0), regulator.adrc_b0_inv);

    iteration_step = fix16_div(F16(INIT_KP_ITERATION_STEP), regulator.adrc_b0_inv);

    regulator.cfg_adrc_p_corr_coeff = F16(MIN_ADRC_P_CORR_COEFF);

    // Set ADRC_KOBSERVERS to safe value
    regulator.cfg_adrc_Kobservers = F16(SAFE_ADRC_KOBSERVERS);

    while (iterations_count < MAX_ITERATIONS)
    {
        speed_tracker.reset();

        // Wait for stable speed with minimal
        // ADRC_KP and safe ADRC_KOBSERVERS
        regulator.cfg_adrc_Kp = fix16_div(F16(MIN_ADRC_KPdivB0), regulator.adrc_b0_inv);
        regulator.adrc_update_observers_parameters();

        ts = GET_TIMESTAMP();
        while (!speed_tracker.is_stable() && (GET_TIMESTAMP() < ts + motor_start_stop_time))
        {
            YIELD_MS(100);
            speed_tracker.push(fix16_from_int(meter.frequency));
        };

        //
        // Measure amplitude
        //

        regulator.cfg_adrc_Kp = adrc_param_attempt_value;
        regulator.adrc_update_observers_parameters();

        measure_amplitude_max_speed = 0;
        measure_amplitude_min_speed = fix16_maximum;

        ts = GET_TIMESTAMP();
        while (GET_TIMESTAMP() < ts + motor_start_stop_time)
        {
            YIELD_MS(100);

            fix16_t f = fix16_from_int(meter.frequency);

            if (measure_amplitude_max_speed < f) measure_amplitude_max_speed = f;
            if (measure_amplitude_min_speed > f) measure_amplitude_min_speed = f;
        }

        fix16_t amplitude = measure_amplitude_max_speed - measure_amplitude_min_speed;

        // Save amplitude of first iteration as reference
        // to compare values of next iterations to this value
        if (iterations_count == 0) first_iteration_amplitude = amplitude;

        // If amplitude is less than margin value
        // step for next iteration should be positive,
        // otherwise - negative
        if (amplitude <= fix16_mul(first_iteration_amplitude, F16(MAX_AMPLITUDE)))
        {
            iteration_step = abs(iteration_step);
        }
        else iteration_step = -abs(iteration_step);

        adrc_param_attempt_value += iteration_step;

        iteration_step /= 2;
        iterations_count++;

    }

    adrc_kp_calibrated_value = fix16_mul(
        adrc_param_attempt_value,
        F16(ADRC_SAFETY_SCALE)
    );

    //--------------------------------------------------------------------------
    // Pick ADRC_KOBSERVERS coeff value by half cut method
    //--------------------------------------------------------------------------

    iterations_count = 0;

    adrc_param_attempt_value = F16(MIN_ADRC_KOBSERVERS);

    iteration_step = F16(INIT_OBSERVERS_ITERATION_STEP);

    // Set ADRC_KP to calibrated value
    regulator.cfg_adrc_Kp = adrc_kp_calibrated_value;

    while (iterations_count < MAX_ITERATIONS)
    {
        speed_tracker.reset();

        // Wait for stable speed with calibrated
        // ADRC_KP and minimal ADRC_KOBSERVERS
        regulator.cfg_adrc_Kobservers = F16(MIN_ADRC_KOBSERVERS);
        regulator.adrc_update_observers_parameters();

        ts = GET_TIMESTAMP();
        while (!speed_tracker.is_stable() && (GET_TIMESTAMP() < ts + motor_start_stop_time))
        {
            YIELD_MS(100);
            speed_tracker.push(fix16_from_int(meter.frequency));
        };

        //
        // Measure amplitude
        //

        regulator.cfg_adrc_Kobservers = adrc_param_attempt_value;
        regulator.adrc_update_observers_parameters();

        measure_amplitude_max_speed = 0;
        measure_amplitude_min_speed = fix16_maximum;

        ts = GET_TIMESTAMP();
        while (GET_TIMESTAMP() < ts + motor_start_stop_time)
        {
            YIELD_MS(100);

            fix16_t f = fix16_from_int(meter.frequency);

            if (measure_amplitude_max_speed < f) measure_amplitude_max_speed = f;
            if (measure_amplitude_min_speed > f) measure_amplitude_min_speed = f;
        }

        fix16_t amplitude = measure_amplitude_max_speed - measure_amplitude_min_speed;

        // Save amplitude of first iteration as reference
        // to compare values of next iterations to this value
        if (iterations_count == 0) first_iteration_amplitude = amplitude;

        // If amplitude is less than margin value
        // step for next iteration should be positive,
        // otherwise - negative
        if (amplitude <= fix16_mul(first_iteration_amplitude, F16(MAX_AMPLITUDE)))
        {
            iteration_step = abs(iteration_step);
        }
        else iteration_step = -abs(iteration_step);

        adrc_param_attempt_value += iteration_step;

        iteration_step /= 2;
        iterations_count++;

    }

    adrc_observers_calibrated_value = fix16_mul(
        adrc_param_attempt_value,
        F16(ADRC_SAFETY_SCALE)
    );

    //--------------------------------------------------------------------------
    // Pick ADRC_P_CORR_COEFF value by half cut method
    //--------------------------------------------------------------------------

    iterations_count = 0;

    adrc_param_attempt_value = F16(MIN_ADRC_P_CORR_COEFF);

    iteration_step = F16(INIT_P_CORR_COEFF_ITERATION_STEP);

    // Set ADRC_KP and ADRC_KOBSERVERS to calibrated values
    regulator.cfg_adrc_Kp = adrc_kp_calibrated_value;
    regulator.cfg_adrc_Kobservers = adrc_observers_calibrated_value;
    regulator.adrc_update_observers_parameters();

    while (iterations_count < MAX_ITERATIONS)
    {
        speed_tracker.reset();

        // Wait for stable speed with calibrated
        // ADRC_KP, calibrated ADRC_KOBSERVERS
        // and minimal ADRC_P_CORR_COEFF
        regulator.cfg_adrc_p_corr_coeff = F16(MIN_ADRC_P_CORR_COEFF);

        ts = GET_TIMESTAMP();
        while (!speed_tracker.is_stable() && (GET_TIMESTAMP() < ts + motor_start_stop_time))
        {
            YIELD_MS(100);
            speed_tracker.push(fix16_from_int(meter.frequency));
        };

        //
        // Measure amplitude
        //

        regulator.cfg_adrc_p_corr_coeff = adrc_param_attempt_value;

        measure_amplitude_max_speed = 0;
        measure_amplitude_min_speed = fix16_maximum;

        ts = GET_TIMESTAMP();
        while (GET_TIMESTAMP() < ts + motor_start_stop_time)
        {
            YIELD_MS(100);

            fix16_t f = fix16_from_int(meter.frequency);

            if (measure_amplitude_max_speed < f) measure_amplitude_max_speed = f;
            if (measure_amplitude_min_speed > f) measure_amplitude_min_speed = f;
        }

        fix16_t amplitude = measure_amplitude_max_speed - measure_amplitude_min_speed;

        // Save amplitude of first iteration as reference
        // to compare values of next iterations to this value
        if (iterations_count == 0) first_iteration_amplitude = amplitude;

        // If amplitude is less than margin value
        // step for next iteration should be positive,
        // otherwise - negative
        if (amplitude <= fix16_mul(first_iteration_amplitude, F16(MAX_P_CORR_COEFF_AMPLITUDE)))
        {
            iteration_step = abs(iteration_step);
        }
        else iteration_step = -abs(iteration_step);

        adrc_param_attempt_value += iteration_step;

        iteration_step /= 2;
        iterations_count++;
    }

    adrc_p_corr_coeff_calibrated_value = fix16_mul(
        adrc_param_attempt_value,
        F16(ADRC_P_CORR_COEFF_SAFETY_SCALE)
    );

    //--------------------------------------------------------------------------
    // Store ADRC params
    //--------------------------------------------------------------------------

    eeprom_float_write(CFG_ADRC_KP_ADDR, fix16_to_float(adrc_kp_calibrated_value));

    eeprom_float_write(CFG_ADRC_KOBSERVERS_ADDR, fix16_to_float(adrc_observers_calibrated_value));

    eeprom_float_write(CFG_ADRC_P_CORR_COEFF_ADDR, fix16_to_float(adrc_p_corr_coeff_calibrated_value));

    //
    // Reload config & flush garbage after unsync, caused by long EEPROM write.
    //
    regulator.configure();
    meter.reset_state();

    //--------------------------------------------------------------------------
    // Calculate noise tresholds
    //--------------------------------------------------------------------------

    // We should be at lowest speed, but wait for sure after regulator reload
    speed_tracker.reset();
    ts = GET_TIMESTAMP();
    while (!speed_tracker.is_stable() && (GET_TIMESTAMP() < ts + motor_start_stop_time))
    {
        YIELD_MS(100);
        speed_tracker.push(fix16_from_int(meter.frequency));
    }

    iterations_count = 0;
    uint64_acc = 0;
    fix16_acc = 0;

    while (iterations_count < 16)
    {
        YIELD_MS(100);
        uint64_acc += meter.magnitude2;
        fix16_acc += regulator.power_out;
    }

    meter.magnitude2_treshold = (uint32_t) ((uint64_acc / 16) * 0.7f);
    regulator.min_power_treshold = fix16_acc / 16;

    eeprom_uint32_write(
        CFG_METER_MAGNITUDE_NOISE_TRESHOLD_ADDR,
        meter.magnitude2_treshold
    );

    eeprom_uint32_write(
        CFG_MIN_POWER_TRESHOLD_ADDR,
        regulator.min_power_treshold
    );

    YIELD_END;
}
