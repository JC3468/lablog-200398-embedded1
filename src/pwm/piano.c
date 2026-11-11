#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define BUZZER_PIN 2
#define TOP 1023        // Resolución de 10 bits
#define BTN_BASE 3      // Pines del 3 al 10 (8 botones de notas)
#define BTN_UP 11
#define BTN_DOWN 12

const uint32_t NOTAS[8] = {
    1046, // Do (C6)
    1175, // Re (D6)
    1318, // Mi (E6)
    1397, // Fa (F6)
    1568, // Sol (G6)
    1760, // La (A6)
    1975, // Si (B6)
    2093  // Do (C7)
};

int main() {
    stdio_init_all();

    // Configuración del pin del Buzzer como PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint chan  = pwm_gpio_to_channel(BUZZER_PIN);

    float f_clk = (float)clock_get_hz(clk_sys);

    pwm_set_wrap(slice, TOP);
    int level = (TOP + 1) / 2; // 50% duty cycle por defecto
    pwm_set_chan_level(slice, chan, level);
    pwm_set_enabled(slice, false); // Comienza apagado

    // Inicializar pines de las 8 notas (GPIO 3 a 10)
    for (int i = 0; i < 8; i++) {
        gpio_init(BTN_BASE + i);
        gpio_pull_up(BTN_BASE + i);
    }

    // Inicializar botones de volumen / nivel
    gpio_init(BTN_UP);
    gpio_pull_up(BTN_UP);
    gpio_init(BTN_DOWN);
    gpio_pull_up(BTN_DOWN);

    while (true) {
        bool note_pressed = false;

        // Leer los 8 botones de notas
        for (int i = 0; i < 8; i++) {
            uint32_t btn_bit = 1u << (BTN_BASE + i);
            if ((sio_hw->gpio_in & btn_bit) == 0) { 
                float div = f_clk / ((float)NOTAS[i] * (float)(TOP + 1));
                pwm_set_clkdiv(slice, div);
                pwm_set_enabled(slice, true);
                note_pressed = true;
                break; 
            }
        }

        // Si ninguna nota está activa, apagar la salida de audio
        if (!note_pressed) {
            pwm_set_enabled(slice, false);
        }

        // Control de nivel / duty cycle con límites
        if ((sio_hw->gpio_in & (1u << BTN_UP)) == 0) {
            level += 64;
            if (level > TOP) level = TOP;
            pwm_set_chan_level(slice, chan, level);
            sleep_ms(150);
        } else if ((sio_hw->gpio_in & (1u << BTN_DOWN)) == 0) {
            level -= 64;
            if (level < 0) level = 0;
            pwm_set_chan_level(slice, chan, level);
            sleep_ms(150);
        }

        sleep_ms(5);
    }

    return 0;
}