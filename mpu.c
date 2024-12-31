#include "mpu.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT  i2c0 

// Function to write a single register
void mpu6050_write_register(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buf, 2, false);
}

// Function to read a single register
uint8_t mpu6050_read_register(uint8_t reg) {
    uint8_t value;
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU6050_ADDR, &value, 1, false);
    return value;
}

void mpu6050_init() {
    // Configure Power Management Register (PWR_MGMT_1)
    // Bit 6: Reset device
    // Bit 3: Disable temperature sensor
    // Bits 2-0: Clock source (0b000 = Internal 8MHz oscillator)
    mpu6050_write_register(MPU6050_PWR_MGMT_1, 0x00);

    // Configure Accelerometer Configuration Register (ACCEL_CONFIG)
    // Bits 4-3: Full scale range (±2g)
    // Other bits set to 0 for default configuration
    mpu6050_write_register(MPU6050_ACCEL_CONFIG, 0x00);

    // Configure DLPF (Digital Low Pass Filter)
    // Bits 2-0: DLPF configuration for low pass filter
    mpu6050_write_register(MPU6050_CONFIG, 0x06);
}

void mpu6050_read_accel(int16_t* x, int16_t* y, int16_t* z) {
    uint8_t buffer[6];
    uint8_t start_reg = MPU6050_ACCEL_XOUT_H;

    // Read 6 bytes of accelerometer data (2 bytes each for X, Y, Z)
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &start_reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU6050_ADDR, buffer, 6, false);
    
    // Combine high and low bytes (16-bit signed integers)
    // Shift high byte left by 8 and OR with low byte
    *x = (int16_t)((buffer[0] << 8) | buffer[1]);
    *y = (int16_t)((buffer[2] << 8) | buffer[3]);
    *z = (int16_t)((buffer[4] << 8) | buffer[5]);
}