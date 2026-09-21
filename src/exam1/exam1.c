#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/rosc.h"
#include "hardware/structs/sio.h"

// --- MAPEO DE PINES ---
#define LED_BASE 0
#define BTN_BASE 9
#define RESTART_BTN 18

// --- R7: VARIABLES COMPARTIDAS CON LA ISR (volatile) ---
volatile uint32_t board_state = 0;   // R2: Estado en un solo entero empacado
volatile bool victory_state = false;
volatile uint32_t rng_state = 1;

// --- R3: MÁSCARAS DE BITS PRECALCULADAS PARA MOVIMIENTOS ---
// Cada máscara representa el LED central y sus vecinos ortogonales.
// Bits: 0 a 8. (0 = Arriba-Izq, 1 = Arriba-Centro, ..., 8 = Abajo-Der)
const uint32_t TOGGLE_MASKS[9] = {
    0x00B, // Pos 0: Invierte bits 0, 1, 3
    0x017, // Pos 1: Invierte bits 0, 1, 2, 4
    0x026, // Pos 2: Invierte bits 1, 2, 5
    0x059, // Pos 3: Invierte bits 0, 3, 4, 6
    0x0BA, // Pos 4: Invierte bits 1, 3, 4, 5, 7
    0x134, // Pos 5: Invierte bits 2, 4, 5, 8
    0x0C8, // Pos 6: Invierte bits 3, 6, 7
    0x1D0, // Pos 7: Invierte bits 4, 6, 7, 8
    0x1A0  // Pos 8: Invierte bits 5, 7, 8
};

// --- R4: RETARDO BASADO EN CICLOS (Sin usar timers por hardware) ---
void delay_cycles(uint32_t cycles) {
    for (volatile uint32_t i = 0; i < cycles; ++i) {
        __asm volatile("nop");
    }
}

// --- R6: GENERADOR DE NÚMEROS ALEATORIOS PROPIO (Sin librerías) ---
// Utiliza algoritmo Xorshift de 32 bits y operaciones a nivel de bits.
uint32_t custom_xorshift32() {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

// Actualiza físicamente los 9 LEDs basándose en el entero board_state (R3)
void update_display() {
    // 0x1FF son 9 bits en uno (0000 0001 1111 1111)
    uint32_t physical_led_mask = 0x1FF << LED_BASE; 
    
    // 2. Apagamos todos los pines correspondientes a los LEDs
    sio_hw->gpio_clr = physical_led_mask; 
    
    // 3. Desplazamos nuestro estado lógico para que coincida con los pines físicos
    // y aplicamos la máscara por seguridad
    sio_hw->gpio_set = (board_state << LED_BASE) & physical_led_mask; 
}

// Genera un tablero aleatorio válido mediante desplazamientos (R3)
void generate_new_board() {
    uint32_t new_board;
    bool is_trivial;

    do {
        new_board = 0;
        
        // 1. Generamos el tablero con la entropía aleatoria
        for (int i = 0; i < 15; i++) {
            uint32_t move_idx = 9;
            while (move_idx > 8) {
                move_idx = custom_xorshift32() & 0x0F; 
            }
            new_board ^= TOGGLE_MASKS[move_idx];
        }

        // 2. Asumimos que el tablero es bueno hasta demostrar lo contrario
        is_trivial = false;

        // CASO A: ¿El tablero ya está resuelto? (0 movimientos)
        if (new_board == 0) {
            is_trivial = true;
        } 
        else {
            // CASO B: ¿Se puede resolver con 1 solo toque?
            // Si el tablero es exactamente igual a la máscara de un solo botón, 
            // significa que presionar ese botón ganará el juego de inmediato.
            for (int i = 0; i < 9; i++) {
                if (new_board == TOGGLE_MASKS[i]) {
                    is_trivial = true;
                    break; 
                }
            }
        }

    // 3. Si resultó ser trivial, repetimos todo el proceso en microsegundos
    } while (is_trivial);

    // Si salimos del ciclo, garantizamos matemáticamente que requiere al menos 2 movimientos
    board_state = new_board;
}

// --- R5 & R8: INTERRUPCIÓN DE HARDWARE PARA REINICIO ---
void restart_isr(uint gpio, uint32_t events) {
    if ((gpio == RESTART_BTN) && (events & GPIO_IRQ_EDGE_FALL)) {
        generate_new_board();
        update_display();
        victory_state = false;
        // Se altera volatile board_state, lo cual actualizará el juego inmediatamente.
    }
    gpio_acknowledge_irq(gpio, events); 
}

void setup_hardware() {
    // 1. Definir máscaras físicas para los pines
    uint32_t led_mask = 0x1FF << LED_BASE; 
    uint32_t btn_mask = (0x1FF << BTN_BASE) | (1 << RESTART_BTN);
    
    // 2. Inicializar todos los pines de juego de golpe
    // Esto conecta físicamente los 19 pines al bloque SIO en una sola instrucción
    gpio_init_mask(led_mask | btn_mask);

    // 3. Configurar direcciones (Output Enable) a nivel de registros SIO
    sio_hw->gpio_oe_set = led_mask; // Configura los 9 LEDs como salidas
    sio_hw->gpio_oe_clr = btn_mask; // Configura los 10 botones como entradas
    
    // 4. Apagar todos los LEDs desde el instante cero
    sio_hw->gpio_clr = led_mask;

    // 5. R1: Configuración EXPLÍCITA de las resistencias (Pulls)
    for (int i = 0; i < 9; i++) {
        gpio_disable_pulls(LED_BASE + i); // LEDs sin pull-up ni pull-down
        gpio_pull_up(BTN_BASE + i);       // Botones de la cuadrícula con pull-up
    }
    gpio_pull_up(RESTART_BTN);            // Botón de reinicio con pull-up

    // R6: Origen de la imprevisibilidad = Hardware Ring Oscillator (ROSC)
    // Se usa el ruido aleatorio del reloj de hardware para inicializar nuestra semilla, 
    // cumpliendo con tener arranques distintos sin requerir interacción del usuario.
    uint32_t seed = 0;
    for(int i = 0; i < 32; i++) {
        seed = (seed << 1) | (rosc_hw->randombit & 1);
        delay_cycles(100); 
    }
    rng_state = (seed == 0) ? 0xACE1 : seed;

    // Configurar y habilitar la interrupción
    gpio_set_irq_enabled_with_callback(RESTART_BTN, GPIO_IRQ_EDGE_FALL, true, &restart_isr);
}

int main() {
    setup_hardware();
    generate_new_board();

    // false = el botón está suelto. true = el botón está siendo mantenido presionado.
    bool btn_flags[9] = {false};

    while (true) {
        // --- MANEJO DE LA VICTORIA (G6) ---
        if (victory_state) {
            // Animación: Conmutar (Toggle) físicamente todos los LEDs usando SIO
            sio_hw->gpio_togl = 0x1FF << LED_BASE; 
            delay_cycles(2000000); 
            
            continue; // Bloquea los botones normales y repite el ciclo para el parpadeo
        }

        update_display();
        
        // --- LECTURA DE BOTONES Y LÓGICA DE JUEGO ---
        for (int i = 0; i < 9; i++) {
            
            // Calculamos el bit físico que le corresponde a este botón
            uint32_t btn_bit = 1u << (BTN_BASE + i); 
            bool is_pressed = ((sio_hw->gpio_in & btn_bit) == 0);
            // Leemos todo el registro SIO y usamos AND lógico para aislar nuestro botón.
            // Como usamos PULL-UP, el botón presionado lee un 0.
            if (is_pressed && !btn_flags[i]) { 
                
                delay_cycles(300000); // Anti-rebote
                
                // Confirmamos que sigue presionado después del anti-rebote
                if ((sio_hw->gpio_in & btn_bit) == 0) {
                    
                    // Aplicamos el movimiento
                    board_state ^= TOGGLE_MASKS[i]; 
                    update_display();
                    
                    // Levantamos la bandera para NO volver a registrarlo hasta que lo suelte
                    btn_flags[i] = true; 
                }
            }
            // CASO 2: El botón NO está presionado Y la bandera está arriba (Recién soltado)
            else if (!is_pressed && btn_flags[i]) {
                
                delay_cycles(300000); // Anti-rebote al soltar
                
                // Bajamos la bandera, dejando el botón listo para un nuevo toque
                btn_flags[i] = false; 
            }
        }

        // --- R3 & G6: DETECCIÓN DE VICTORIA ---
        if (board_state == 0) {
            victory_state = true;
        }
    }
}