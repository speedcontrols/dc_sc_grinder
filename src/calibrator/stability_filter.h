#ifndef __STABILITY_FILTER_TEMPLATE__
#define __STABILITY_FILTER_TEMPLATE__


#include "libfixmath/fix16.h"


// Sliding tracker to wait until input value become stable.
//
// 1. Apply median filter first (if enabled).
// 2. Test deviation of median filter output.
//
template <fix16_t PRECISION_IN_PERCENTS, uint8_t FILTER_LENGTH = 3>
class StabilityFilterTemplate {

public:
    StabilityFilterTemplate() {
        reset();
    }

    void reset()
    {
        head_idx = 0;
        data_count = 0;
    }

    void push(fix16_t val) {
        data[head_idx++] = val;
        if (head_idx == FILTER_LENGTH) head_idx = 0;
        data_count++;
        return;
    }

    bool is_stable() {
        if (data_count < FILTER_LENGTH) return false;

        fix16_t min = fix16_maximum;
        fix16_t max = fix16_minimum;

        // Temporary hardcoded for fixed length (3)
        for (uint8_t i = 0; i < FILTER_LENGTH; i++)
        {
            fix16_t val = data[i];

            if (val < min) min = val;
            if (val > max) max = val;
        }

        fix16_t diff = max - min;

        fix16_t abs_max = max > 0 ? max : - max;

        if (fix16_mul(abs_max, edge_multiplier) < diff) return false;

        return true;
    }

    fix16_t average() {
        fix16_t sum = 0;

        for (uint8_t i = 0; i < FILTER_LENGTH; i++)
        {
            sum += data[i];
        }

        return fix16_mul(sum, F16(1.0 / FILTER_LENGTH));
    }


private:
    fix16_t data[FILTER_LENGTH];
    int head_idx;
    int data_count;

    fix16_t edge_multiplier = PRECISION_IN_PERCENTS / 100;
};

#endif
