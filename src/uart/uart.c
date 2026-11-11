#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <stdio.h>
 
#define UART_ID uart0
#define BAUD_RATE 115200
#define TX_PIN 0
#define RX_PIN 1
#define Button_A 12
#define Button_B 13
#define Button_C 14
#define Led_A 16
#define Led_B 17
#define Led_C 18
 
int main() {
    stdio_init_all();
 
    gpio_init(Button_A);
    gpio_init(Button_B);
    gpio_init(Button_C);
    gpio_init(Led_A);
    gpio_init(Led_B);
    gpio_init(Led_C);
 
    gpio_set_dir(Led_A, 1);
    gpio_set_dir(Led_B, 1);
    gpio_set_dir(Led_C, 1);
    gpio_set_dir(Button_A, 0);
    gpio_set_dir(Button_B, 0);
    gpio_set_dir(Button_C, 0);
 
    gpio_pull_up(Button_A);
    gpio_pull_up(Button_B);
    gpio_pull_up(Button_C);
 
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);
 
    uart_init(UART_ID, BAUD_RATE);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
 
    printf("[MAIN] Sistema iniciado...\n");
 
    int flag_a = 0;
    int flag_b = 0;
    int flag_c = 0;
 
    while (true) {
        // --- BOTON A ---
        if (gpio_get(Button_A) == 0 && flag_a == 0) {
            uart_putc(UART_ID, 'A');  // Se presionó
            flag_a = 1;
        } else if (gpio_get(Button_A) == 1 && flag_a == 1) {
            uart_putc(UART_ID, 'a');  // Se soltó
            flag_a = 0;
        }
 
        // --- BOTON B ---
        if (gpio_get(Button_B) == 0 && flag_b == 0) {
            uart_putc(UART_ID, 'B');  // Se presionó
            flag_b = 1;
        } else if (gpio_get(Button_B) == 1 && flag_b == 1) {
            uart_putc(UART_ID, 'b');  // Se soltó
            flag_b = 0;
        }
        
        // --- BOTON C ---
        if (gpio_get(Button_C) == 0 && flag_c == 0) {
            uart_putc(UART_ID, 'C');  // Se presionó
            flag_c = 1;
        } else if (gpio_get(Button_C) == 1 && flag_c == 1) {
            uart_putc(UART_ID, 'c');  // Se soltó
            flag_c = 0;
        }
        
        // --- RECEPCION UART ---
        while (uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);

            // Control Led A
            if (c == 'A') {
                gpio_put(Led_A, 1);
            } else if (c == 'a') {
                gpio_put(Led_A, 0);
            }

            // Control Led B
            if (c == 'B') {
                gpio_put(Led_B, 1);
            } else if (c == 'b') {
                gpio_put(Led_B, 0);
            }

            // Control Led C
            if (c == 'C') {
                gpio_put(Led_C, 1);
            } else if (c == 'c') {
                gpio_put(Led_C, 0);
            }
        }

        sleep_ms(10);
    }
}