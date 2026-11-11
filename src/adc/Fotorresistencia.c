#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define LDR_ADC_INPUT 0          // Canal 0 -> GPIO26
#define LDR_GPIO_PIN 26          // Pin donde se conecta el divisor de voltaje del LDR
const float alpha = 0.0625f;     // Filtro EMA (0.0625 = 1/16) para estabilizar la lectura

const uint16_t LDR_MIN = 9;
const uint16_t LDR_MAX = 3600;

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

        if (!y_init) {
            y = (float)x;          
            y_init = true;
        } else {
            y = alpha * (float)x + (1.0f - alpha) * y;  
        }

        uint16_t y_u16 = (uint16_t)(y + 0.5f);

        // Cálculos útiles para fotorresistencias
        float voltaje = (y_u16 * 3.3f) / 4095.0f; 
        int porcentaje = ((y_u16 - LDR_MIN) * 100) / (LDR_MAX - LDR_MIN);
        //if (porcentaje < 0) porcentaje = 0;
        //if (porcentaje > 100) porcentaje = 100;

        // Imprime los datos incluyendo el voltaje y el porcentaje estimado
        printf("LDR Raw=%u | Filtro=%u | Voltaje=%.2fV | Nivel=%d%%\n", x, y_u16, voltaje, porcentaje);
        
        sleep_ms(100); // Aumentado a 100ms para no saturar la terminal (10 lecturas/segundo)
    }
}