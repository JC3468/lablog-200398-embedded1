#include "pico/stdlib.h"
#include "hardware/structs/sio.h"
#include <stdio.h>

#define PIN_A 8
#define PIN_B 7
#define PIN_C 6
#define PIN_D 5
#define PIN_E 4

#define PIN_BA 16
#define PIN_BB 17
#define PIN_BC 18


volatile int speed = 500;
volatile int counter = 0;


void blink_blocking(uint led_pin, int times, int ms_delay) {
    for (int i = 0; i < times; ++i) {
        gpio_xor_mask(1u << led_pin);
        busy_wait_ms(ms_delay);   // intentional blocking to make the "busyness" noticeable
        gpio_xor_mask(1u << led_pin);
        busy_wait_ms(ms_delay);
    }
}

static void gpio_isr(uint gpio, uint32_t events) {
    if  (events & GPIO_IRQ_EDGE_FALL) {
        if  (gpio == PIN_BC && counter == 2) {
        sio_hw->gpio_clr = (1u << PIN_A) |
        (1u << PIN_B) |
        (1u << PIN_C) |
        (1u << PIN_D) |
        (1u << PIN_E);
        blink_blocking(6,50,300);
    }
        else if (gpio == PIN_BB){
        speed = speed + 50;
        }
        else if (gpio == PIN_BA){
            if (speed >= 100){
        speed = speed - 50;}
        }
    }
    gpio_acknowledge_irq(gpio, events);  // clears the IRQ flag
}





int main(void) {
    stdio_init_all();

    gpio_init(PIN_A);
    gpio_init(PIN_B);
    gpio_init(PIN_C);
    gpio_init(PIN_D);
    gpio_init(PIN_E);

    gpio_init(PIN_BA);
    gpio_init(PIN_BB);
    gpio_init(PIN_BC);


    gpio_pull_up(PIN_BA);
    gpio_pull_up(PIN_BB);
    gpio_pull_up(PIN_BC);

    const uint32_t MASKB =
        (1u << PIN_BA) |
        (1u << PIN_BB) |
        (1u << PIN_BC);

    const uint32_t MASK =
        (1u << PIN_A) |
        (1u << PIN_B) |
        (1u << PIN_C) |
        (1u << PIN_D) |
        (1u << PIN_E);

    // OE = Output Enable
    sio_hw->gpio_oe_set = MASK;
    sio_hw->gpio_oe_clr = MASKB;


    gpio_set_irq_enabled_with_callback(PIN_BA,
                                       GPIO_IRQ_EDGE_FALL,
                                       true,
                                       &gpio_isr);

    gpio_set_irq_enabled(PIN_BB,
                        GPIO_IRQ_EDGE_FALL,
                        true);
    
    gpio_set_irq_enabled(PIN_BC,
                        GPIO_IRQ_EDGE_FALL,
                        true);

    while (true) {
        sio_hw->gpio_clr = MASK;
        sio_hw->gpio_set = (1u << (counter + 4));
        sleep_ms(speed);

        
        counter++;
        if (counter>4){
            while (counter > 0){
                counter--,
                sio_hw->gpio_clr = MASK;

                sio_hw->gpio_set = (1u << (counter + 4));
                sleep_ms(speed);}
            
        }
    }
}