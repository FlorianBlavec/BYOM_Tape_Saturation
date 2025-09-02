/**
 * Hey i'm Florian Blavec :)
 *
 * Description : Tape Saturation, upper pot is saturation amount, bottom is output level
 */

#include "pico/stdlib.h"
#include <stdlib.h>
#include "hardware/irq.h"  // interrupts
#include "hardware/pwm.h"  // pwm 
#include "hardware/sync.h" // wait for interrupt 
#include "hardware/adc.h" // adc_*()
#include <math.h>

#define LED_PIN1 3
#define LED_PIN2 4
#define AUDIO_OUT 6
#define AUDIO_IN 26
#define PARAM1 27
#define PARAM2 28
#define SAT_FACTOR 100.0
// ADC Pico on 12 bits (0-4095)
#define ADC_MAX 4095.0
#define PWM_WRAP_VALUE 1250

//input Pin 26 value
double raw;

//input Pin 27 value
double saturation;

//input Pin 28 value
uint16_t out_level;

void pwm_interrupt_handler() {
    //Read ADC inputs for audio and pots
    adc_select_input(0);
    raw = adc_read();
    adc_select_input(1);
    saturation = adc_read();
    adc_select_input(2);
    out_level = adc_read();
    
     // Normalize ADC (12 bits on Pico)
    double raw_normalized = (double)raw / 4095.0;  // [0, 1]
    double sat_normalized = (double)saturation / 4095.0;  // [0, 1]
    double level_normalized = (double)out_level / 4095.0;  // [0, 1]
    
    // Centrer le signal audio sur 0
    double input = (raw_normalized - 0.5) * 2.0;  // [-1, 1]
    
    // Saturation 1 : Tape Saturation
    double gain = 1.0 + sat_normalized * SAT_FACTOR;
    double saturated = tanh(input * gain);
    
    // Saturation 2 : Exponential Saturation
    // double x = input * (1.0 + sat_normalized * SAT_FACTOR);
    // double saturated;
    // if (fabs(x) < 1.0) {
    //     saturated = x;
    // } else {
    //     saturated = (x > 0) ? 1.0 - exp(-x) : -1.0 + exp(x);
    // }
    
    // Saturation 3: Overdrive
    // double drive = 1.0 + sat_normalized * SAT_FACTOR;
    // double x = input * drive;
    // double saturated;
    // if (fabs(x) < 0.33) {
    //     saturated = 2.0 * x;
    // } else if (fabs(x) < 0.66) {
    //     double sign = (x > 0) ? 1.0 : -1.0;
    //     saturated = sign * (3.0 - pow(2.0 - 3.0 * fabs(x), 2.0)) / 3.0;
    // } else {
    //     saturated = (x > 0) ? 1.0 : -1.0;
    // }
    
    // Saturation 4: Tube Saturation
    // double tube_gain = 1.0 + sat_normalized * SAT_FACTOR;
    // double biased = input + 0.1 * sat_normalized;  // Bias asymétrique
    // double x = biased * tube_gain;
    // double saturated = x / (1.0 + fabs(x));  // Soft clipping simple
    
    // get back to [0, 1]
    double output = (saturated + 1.0) * 0.5;
    
    // gain stage
    output *= level_normalized;
    
    // Convert to PWM (0 : 1250)
    uint16_t pwm_value = (uint16_t)(output * 1250.0);
    if (pwm_value > 1250) pwm_value = 1250;
    
    // Display saturation level on LED1
    uint16_t pwm_sat = (uint16_t)(sat_normalized * 1250.0);
    
    pwm_set_gpio_level(LED_PIN1, pwm_sat);
    pwm_set_gpio_level(LED_PIN2, pwm_value);
    pwm_set_gpio_level(AUDIO_OUT, pwm_value);
    
    pwm_clear_irq(pwm_gpio_to_slice_num(LED_PIN1));
    pwm_clear_irq(pwm_gpio_to_slice_num(LED_PIN2));
    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_OUT));
}

int main(void) {
    adc_init();
    adc_gpio_init(AUDIO_IN);
    adc_gpio_init(PARAM1);
     adc_gpio_init(PARAM2);
    /// \tag::setup_pwm[]
    gpio_set_function(LED_PIN1, GPIO_FUNC_PWM);
    gpio_set_function(LED_PIN2, GPIO_FUNC_PWM);
    gpio_set_function(AUDIO_OUT, GPIO_FUNC_PWM);
    int led1_pin_slice = pwm_gpio_to_slice_num(LED_PIN1);
    int led2_pin_slice = pwm_gpio_to_slice_num(LED_PIN2);
    int audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_OUT);

    pwm_clear_irq(led1_pin_slice);
    pwm_set_irq_enabled(led1_pin_slice, true);
    pwm_clear_irq(led2_pin_slice);
    pwm_set_irq_enabled(led2_pin_slice, true);
    pwm_clear_irq(audio_pin_slice);
    pwm_set_irq_enabled(audio_pin_slice, true);
    
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_interrupt_handler);
    irq_set_enabled(PWM_IRQ_WRAP, true);
    
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);
    pwm_config_set_wrap(&config, 1250); 
    pwm_init(led1_pin_slice, &config, true);
    pwm_init(led2_pin_slice, &config, true);
    pwm_init(audio_pin_slice, &config, true);
    /// \end::setup_pwm[]
    pwm_set_gpio_level(LED_PIN1, 0);
    pwm_set_gpio_level(LED_PIN2, 0);
    pwm_set_gpio_level(AUDIO_OUT, 0);

    while(1) {
        __wfi(); // Wait for Interrupt
    }
}
