#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT    i2c0
#define SDA_PIN     4
#define SCL_PIN     5
#define RTC_ADDR 0x68 // 7-bit address of the DS1307 RTC
#define MPU_ADDR 0x69 // MPU6050 with AD0 = 1 (0x68 is taken by the RTC)

int main(void) {
    stdio_init_all();
    // Initialize i2c at a speed of 100 kHz
    i2c_init(I2C_PORT, 100000);
    // Configure the pins to work as i2c
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    // Enable internal pull-ups on the pins
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    sleep_ms(500);
    uint8_t reg = 0x00;     // RTC: first register (seconds)
    uint8_t regmpu = 0x3B;  // MPU6050: ACCEL_XOUT_H
    uint8_t memory[7];      // RTC: sec, min, hour, day, date, month, year
    uint8_t memory2[14];    // MPU6050: accel(6) + temp(2) + gyro(6)

    while (true) {
        // START + addr|W + WRITE(reg) + ACK  + NoStop
        int w = i2c_write_blocking(I2C_PORT, RTC_ADDR, &reg, 1, /*nostop=*/true);
        if (w < 0) {
            printf("I2C error (ret=%d)\n", w);
            goto next;
        } else if (w != 1) {
            printf("Partial write: %d/1 bytes\n", w);
            goto next;
        } else {
            printf("Written successfully\n");
        }
        // REPEATED START + addr|R + READ(data) + NACK + STOP
        int r = i2c_read_blocking(I2C_PORT, RTC_ADDR, memory, sizeof(memory), /*nostop=*/false);
        if (r < 0) {
            printf("I2C error (ret=%d)\n", r);
        } else if (r != (int)sizeof(memory)) {
            printf("Partial read: %d/%d bytes\n", r, (int)sizeof(memory));
            printf("Data read: ");
            for (int i = 0; i < r; i++) {
                printf("0x%02X ", memory[i]);
            }
        } else {
            printf("Read successfully\n");
            printf("Data read: \n");
            printf("%02X:%02X:%02X \n", memory[2], memory[1], memory[0]);
            printf("%02X/%02X/%02X \n", memory[4], memory[5], memory[6]);
        }
        next:
            sleep_ms(1000);

        // START + addr|W + WRITE(reg) + ACK  + NoStop
        int w2 = i2c_write_blocking(I2C_PORT, MPU_ADDR, &regmpu, 1, /*nostop=*/true);
        if (w2 < 0) {
            printf("I2C error (ret=%d)\n", w2);
            goto next2;
        } else if (w2 != 1) {
            printf("Partial write: %d/1 bytes\n", w2);
            goto next2;
        } else {
            printf("Written successfully\n");
        }
        // REPEATED START + addr|R + READ(data) + NACK + STOP
        int r2 = i2c_read_blocking(I2C_PORT, MPU_ADDR, memory2, sizeof(memory2), /*nostop=*/false);
        if (r2 < 0) {
            printf("I2C error (ret=%d)\n", r2);
        } else if (r2 != (int)sizeof(memory2)) {
            printf("Partial read: %d/%d bytes\n", r2, (int)sizeof(memory2));
            printf("Data read: ");
            for (int i = 0; i < r2; i++) {
                printf("0x%02X ", memory2[i]);
            }
        } else {
            // 1. Join high and low bytes (big-endian)
            int16_t accel_x_raw = (memory2[0]  << 8) | memory2[1];
            int16_t accel_y_raw = (memory2[2]  << 8) | memory2[3];
            int16_t accel_z_raw = (memory2[4]  << 8) | memory2[5];

            int16_t temp_raw    = (memory2[6]  << 8) | memory2[7];

            int16_t gyro_x_raw  = (memory2[8]  << 8) | memory2[9];
            int16_t gyro_y_raw  = (memory2[10] << 8) | memory2[11];
            int16_t gyro_z_raw  = (memory2[12] << 8) | memory2[13];

            // 2. Convert to physical units
            // Default scale: accel = +/-2g (16384 LSB/g), gyro = +/-250 deg/s (131 LSB/deg/s)
            float accel_x = accel_x_raw / 16384.0;
            float accel_y = accel_y_raw / 16384.0;
            float accel_z = accel_z_raw / 16384.0;

            // Datasheet formula for temperature in deg C
            float temperatura = (temp_raw / 340.0) + 36.53;

            float gyro_x = gyro_x_raw / 131.0;
            float gyro_y = gyro_y_raw / 131.0;
            float gyro_z = gyro_z_raw / 131.0;

            // 3. Print readable values
            printf("Read successfully\n");
            printf("Data read: \n");
            printf("Aceleracion (G): X: %.2f, Y: %.2f, Z: %.2f\n", accel_x, accel_y, accel_z);
            printf("Giroscopio (°/s): X: %.2f, Y: %.2f, Z: %.2f\n", gyro_x, gyro_y, gyro_z);
            printf("Temperatura:     %.1f °C\n", temperatura);
        }
        next2:
            sleep_ms(1000);
    }
    return 0;
}
