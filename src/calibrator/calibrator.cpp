#include "calibrator.h"
#include "../yield.h"
#include "../app.h"
#include "../eeprom.h"

void Calibrator::configure()
{
    done = (bool) eeprom_uint32_read(CFG_CALIBRATION_DONE_ADDR, 0);
}

bool Calibrator::tick()
{
    YIELDABLE;

    YIELD_WHILE(!wait_knob_dial());

    active = true;
    regulator.disable();
    meter.magnitude2_treshold = 0;

    YIELD_WHILE(!calibrate_adrc());
    regulator.enable();

    // Flash at the end of calibration
    regulator.setpoint = F16(1.0f);
    YIELD_MS(150);
    regulator.setpoint = F16(0.1f);
    YIELD_MS(150);
    regulator.setpoint = F16(1.0f);
    YIELD_MS(150);
    regulator.setpoint = F16(0.1f);
    YIELD_MS(150);
    regulator.setpoint = F16(1.0f);
    YIELD_MS(150);
    regulator.setpoint = F16(0.1f);
    YIELD_MS(150);

    eeprom_uint32_write(CFG_CALIBRATION_DONE_ADDR, 1);
    done = true;
    active = false;

    YIELD_END;
}


#define KNOB_TRESHOLD F16(0.05)

#define IS_KNOB_LOW(val)  (val < KNOB_TRESHOLD)
#define IS_KNOB_HIGH(val) (val >= KNOB_TRESHOLD)

#define KNOB_WAIT_MIN_MS 200
#define KNOB_WAIT_MAX_MS 1000

bool Calibrator::wait_knob_dial() {
    YIELDABLE;

    // Try endless
    while (1)
    {
        // First, check knob is at start position (zero),
        // prior to start detect dial sequence
        YIELD_MS(1);

        YIELD_WHILE(IS_KNOB_LOW(io.knob));

        if (YIELD_GET_MS() < KNOB_WAIT_MIN_MS) continue;

        // If knob is zero long enough => can start detect dials
        dials_cnt = 0;

        while (1) {
            // Measure UP interval

            YIELD_WHILE(IS_KNOB_HIGH(io.knob));

            uint32_t interval = YIELD_GET_MS();

            // Restart on invalid length
            if (interval < KNOB_WAIT_MIN_MS || interval > KNOB_WAIT_MAX_MS) break;

            // Finish on success
            // (return without YIELD also resets state to start)
            if (++dials_cnt >= 3) {
                YIELD_END;
            }

            // Measure DOWN interval
            YIELD_WHILE(IS_KNOB_LOW(io.knob));

            interval = YIELD_GET_MS();

            // Restart on invalid length
            if (interval < KNOB_WAIT_MIN_MS || interval > KNOB_WAIT_MAX_MS) break;
        }
    }
}
