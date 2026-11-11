// Timers lab — repeating timer on GP15 (course code, Bloque 2 / Tema F)
// Only change between runs: TOGGLE_MS (250 for E1, 10 for E2/E3).
#include <stdio.h>
#include "pico/stdlib.h"

#define OUT_PIN    15    // GP15 -> scope CH1
#define TOGGLE_MS  250   // half-period; T = 2 * TOGGLE_MS, f = 1000 / (2 * TOGGLE_MS)

static volatile bool level = false;

static bool toggle_cb(repeating_timer_t *t) {
    (void)t;
    level = !level;
    gpio_put(OUT_PIN, level);   // keep the callback short
    return true;                // keep repeating
}

int main(void) {
    stdio_init_all();

    gpio_init(OUT_PIN);
    gpio_set_dir(OUT_PIN, GPIO_OUT);
    gpio_put(OUT_PIN, 0);

    repeating_timer_t timer;
    // Negative delay: callbacks are scheduled start-to-start, so the period
    // does not grow with the callback's execution time.
    add_repeating_timer_ms(-TOGGLE_MS, toggle_cb, NULL, &timer);

    while (true) {
        tight_loop_contents();  // main() is free; the hardware timer keeps time
    }
}
