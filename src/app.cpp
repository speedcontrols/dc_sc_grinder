#include <stdint.h>
#include "app.h"
#include "app_hal.h"

#include "io.h"
#include "meter.h"
#include "regulator.h"
#include "eeprom.h"
#include "config.h"
#include "calibrator/calibrator.h"
#include "libfixmath/fix16.h"


Io io;
Meter meter;
Calibrator calibrator;
Regulator regulator;

int main()
{
    hal::setup();

    // Load config info from emulated EEPROM
    //regulator.configure();

    meter.configure();
    calibrator.configure();
    regulator.configure();

    if (calibrator.done) regulator.disable();
    else hal::set_power(F16(NOT_CALIBRATED_MOTOR_POWER));

    while (1) {
        if (!io.out.empty())
        {
            io_data_t io_data;
            io.out.pop(io_data);

            if (meter.consume(io_data))
            {
                // if frequency was recalculated - drop overflowed queue
                // and pass new value to regulator
                regulator.freq_in = meter.frequency;
                io.out.clear();
            }

            calibrator.tick();

            // Detach knob on calibration
            if (!calibrator.active) regulator.apply_knob(io.knob);
        }
    }
}
