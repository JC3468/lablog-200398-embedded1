#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define LDR_ADC_INPUT 0          // Canal 0 -> GPIO26
#define LDR_GPIO_PIN 26          // Pin donde se conecta el divisor de voltaje del electret




int main() {
    stdio_init_all();
    adc_init();
    adc_gpio_init(LDR_GPIO_PIN); 
    adc_select_input(LDR_ADC_INPUT);

    bool y_init = false;
    float y = 0.0f;

    printf("Iniciando lectura de Fotorresistencia (LDR)...\n");

    while (true) {
        uint16_t x = adc_read();   // Lectura cruda (0..4095)

        if (x>2110) {
        printf("LDR Raw=%u | Ruido alto\n", x);
        sleep_ms(100); // Aumentado a 100ms para no saturar la terminal (10 lecturas/segundo)
    }
        else{
        printf("Electret Raw=%u | Ruido normal\n", x);
        sleep_ms(100); // Aumentado a 100ms para no saturar la terminal (10 lecturas/segundo)
        }
}
}
