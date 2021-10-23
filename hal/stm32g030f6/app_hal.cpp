#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "gpio.h"

#include "app_hal.h"
#include "app.h"

extern "C" void SystemClock_Config(void);


namespace hal {

// ADC data is transferred to double size DMA buffer. Interrupts happen on half
// transfer and full transfer. So, we can process received data without risk
// of override. While half of buffer is processed, another half os used to
// collect next data.

static volatile uint16_t ADCBuffer[ADC_FETCH_PER_TICK * ADC_CHANNELS_COUNT * 2];

// Resorted adc data for convenient use
static uint16_t adc_current_buf[ADC_FETCH_PER_TICK];
static uint16_t adc_knob_buf[ADC_FETCH_PER_TICK];

// Split raw ADC data by separate buffers
static void adc_raw_data_load(uint32_t adc_data_offset)
{
    for (int sample = 0; sample < ADC_FETCH_PER_TICK; sample++)
    {
        adc_current_buf[sample] = ADCBuffer[adc_data_offset++];
        adc_knob_buf[sample] = ADCBuffer[adc_data_offset++];
    }
}

void on_adc_half_transfer_done(ADC_HandleTypeDef* AdcHandle)
{
    (void)(AdcHandle);
    adc_raw_data_load(0);
    io.consume(adc_current_buf, adc_knob_buf);
}

void on_adc_transfer_done(ADC_HandleTypeDef* AdcHandle)
{
    (void)(AdcHandle);
    adc_raw_data_load(ADC_FETCH_PER_TICK * ADC_CHANNELS_COUNT);
    io.consume(adc_current_buf, adc_knob_buf);
}

// PWM period
#define PWM_TIMER_CYCLES 2929

// Set PWM duty cycle, 0..1
void set_power(fix16_t duty_cycle)
{
    static fix16_t prev = -1;

    // Clamp value
    fix16_t val = duty_cycle;
    if (val > fix16_one) val = fix16_one;
    if (val < 0) val = 0;

    // Exit if nothing changed
    if (val == prev) return;

    __HAL_TIM_SET_COMPARE(
        &htim1,
        TIM_CHANNEL_1,
        uint16_t((val * PWM_TIMER_CYCLES) >> 16)
    );
}

// HW init
void setup(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_DMA_Init();
    MX_TIM1_Init();
    MX_TIM14_Init();

    set_power(0);

    HAL_ADC_RegisterCallback(
        &hadc1,
        HAL_ADC_CONVERSION_HALF_CB_ID,
        on_adc_half_transfer_done
    );

    HAL_ADC_RegisterCallback(
        &hadc1,
        HAL_ADC_CONVERSION_COMPLETE_CB_ID,
        on_adc_transfer_done
    );

    HAL_ADCEx_Calibration_Start(&hadc1);

    HAL_ADC_Start_DMA(
        &hadc1,
        (uint32_t*)ADCBuffer,
        ADC_FETCH_PER_TICK * ADC_CHANNELS_COUNT * 2
    );
}

}
