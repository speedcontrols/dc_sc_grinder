#ifndef __CONFIG__
#define __CONFIG__

#include "libfixmath/fix16.h"

// Amount of motor poles, required to convert frequency to RPM
#define MOTOR_POLES 8

// If calibration not done on fresh device, power motor at low speed
// to indicate mode
#define NOT_CALIBRATED_MOTOR_POWER 0.2f

// Minimal allowed speed (RPM).
// For rectified power, frequency should NOT be < 550Hz (min_rpm * MOTOR_POLES / 60)
#define MOTOR_MIN_RPM_LIMIT 5000.0f

// Max allowed motor speed (not used now)
// Set desired value, if you wish to restrict speed below max available
#define MOTOR_MAX_RPM_LIMIT 100000.0f

// Knob initial zone where motor should not run.
#define KNOB_DEAD_ZONE_WIDTH  0.02f

//
// Virtual EEPROM addresses & defaults for config variables.
// Emulator uses append-only log & multiphase commits to guarantee
// atomic writes.
//
// Params below are filled by calibrator
//

#define CFG_RPM_MAX_ADDR 1
#define CFG_RPM_MAX_DEFAULT 30000.0f

#define CFG_CALIBRATION_DONE_ADDR 2

#define CFG_METER_MAGNITUDE_NOISE_TRESHOLD_ADDR 3

#define CFG_MIN_POWER_TRESHOLD_ADDR 4

// ADRC parameters (auto-calibrated).
#define CFG_ADRC_KP_ADDR 5
#define CFG_ADRC_KP_DEFAULT 1.0f

#define CFG_ADRC_KOBSERVERS_ADDR 6
#define CFG_ADRC_KOBSERVERS_DEFAULT 1.0f

#define CFG_ADRC_P_CORR_COEFF_ADDR 7
#define CFG_ADRC_P_CORR_COEFF_DEFAULT 0.0f

#endif
