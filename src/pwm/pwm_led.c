// PWM lab — LED fade / fixed duty (course code pwm_led.c, Bloque 2 / Tema G)
// NOTE: reconstructed after the session from the course example + the scope
// captures (the exact file used in the lab was not saved). The variants used
// for each capture are listed in docs/sessions/2026-09-30-pwm.md.
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define LED_PIN   2          // GP2 -> LED + scope CH1
#define F_PWM_HZ  2000       // target PWM frequency
#define TOP       1023       // 10-bit: level 0..1023  (DIV_10 / DIV_1 runs: 4095)

// Mode: 1 = fade 0 -> TOP -> 0 (course example), 0 = fixed duty
#define FADE      1
#define DUTY_PCT  20         // fixed-duty runs: 35, 87, 20

int main(void) {
    stdio_init_all();

    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(LED_PIN);
    uint chan  = pwm_gpio_to_channel(LED_PIN);

    float f_clk = 150000000.0f;                       // or clock_get_hz(clk_sys)
    float div   = f_clk / (F_PWM_HZ * (TOP + 1));     // = 73.24 for 2 kHz / TOP 1023
    pwm_set_clkdiv(slice, div);   // HW keeps 8.4 fixed point: 73.24 -> 73 + 3/16
    // pwm_set_clkdiv(slice, 10.0f);  // DIV_10 run (TOP = 4095)
    // pwm_set_clkdiv(slice, 1.0f);   // DIV_1  run (TOP = 4095)
    pwm_set_wrap(slice, TOP);

    pwm_set_chan_level(slice, chan, 0);
    pwm_set_enabled(slice, true);

#if FADE
    int level = 0, step = 8;
    while (true) {
        pwm_set_chan_level(slice, chan, level);
        sleep_ms(5);
        level += step;
        if (level >= TOP || level <= 0) step = -step;  // bounce 0 <-> TOP
    }
#else
    uint16_t level = (uint16_t)((DUTY_PCT * (TOP + 1) + 50) / 100);  // 20 % -> 205
    pwm_set_chan_level(slice, chan, level);
    while (true) tight_loop_contents();
#endif
}
