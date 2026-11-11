#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <math.h>

#define Temp_INPUT 0          // channel 0 -> GPIO26
#define Pot_INPUT 1 // channel 0 -> GPIO27
const float alpha = 0.0625f; // 0<alpha<=1 (0.0625 = 1/16)

const float BETA = 3950.0f;

int main() {
    stdio_init_all();
    adc_init();
    adc_gpio_init(26);        // GPIO26 = ADC0
    adc_gpio_init(27);

    bool   p_init = false;
    float  p = 0.0f;

    bool   t_init = false;
    float  t = 0.0f;

    while (true) {
        adc_select_input(Pot_INPUT);
        uint16_t xp = adc_read();   // 0..4095 (12 bits)

        if (!p_init) {
            p = (float)xp;          // initialize with the first reading
            p_init = true;
        } else {
            p = alpha * (float)xp + (1.0f - alpha) * p;  // y(n) = αx + (1-α)y
        }

        // If you prefer an integer for printing/outputs:
        uint16_t p_u16 = (uint16_t)(p + 0.5f);
        uint16_t grados = p_u16 * 270 / 4095;

        printf("raw=%u ema=%u Degree=%u\n", xp, p_u16, grados);

        adc_select_input(Temp_INPUT);
        uint16_t xt = adc_read();   // 0..4095 (12 bits)

        if (!t_init) {
            t = (float)xt;          // initialize with the first reading
            t_init = true;
        } else {
            t = alpha * (float)xt + (1.0f - alpha) * t;  // y(n) = αx + (1-α)y
        }

        // If you prefer an integer for printing/outputs:
        uint16_t t_u16 = (uint16_t)(t + 0.5f);

        double analogValue = (double)t_u16;
        float temperatura = (float)(1.0 / (log(1.0 / (4095.0 / analogValue - 1.0)) / (double)BETA + 1.0 / 298.15) - 273.15);

        printf("raw=%u ema=%d temp=%.2f\n", xt, t_u16, temperatura);
        sleep_ms(10);
    }
}
