#include "i2c.h"
#include "pico/stdlib.h"

// Cấu hình I2C
void i2c_init_custom() {
    // Bật I2C
    *(volatile uint32_t*)I2C0_CTRL = I2C_CTRL_EN;

    // Cấu hình tốc độ I2C (100kHz)
    *(volatile uint32_t*)I2C0_DIV = 250; // Giá trị này để đạt tốc độ khoảng 100kHz
}

// Gửi lệnh I2C
void i2c_write(uint8_t addr, uint8_t *data, size_t len) {
    // Đặt địa chỉ thiết bị và gửi lệnh
    *(volatile uint32_t*)I2C0_FIFO = addr;

    // Gửi dữ liệu
    for (size_t i = 0; i < len; i++) {
        *(volatile uint32_t*)I2C0_FIFO = data[i];
        while (*(volatile uint32_t*)I2C0_STATUS & 0x02); // Đợi cho đến khi dữ liệu được ghi xong
    }
    
    // Gửi tín hiệu STOP
    *(volatile uint32_t*)I2C0_CTRL |= I2C_CTRL_STOP;
}

// Đọc dữ liệu I2C
void i2c_read(uint8_t addr, uint8_t *data, size_t len) {
    // Đặt địa chỉ thiết bị và gửi lệnh đọc
    *(volatile uint32_t*)I2C0_FIFO = addr | 0x01;

    // Đọc dữ liệu
    for (size_t i = 0; i < len; i++) {
        while (!( *(volatile uint32_t*)I2C0_STATUS & I2C_STATUS_RX_READY)); // Đợi dữ liệu có sẵn
        data[i] = *(volatile uint32_t*)I2C0_FIFO;
    }

    // Gửi tín hiệu STOP
    *(volatile uint32_t*)I2C0_CTRL |= I2C_CTRL_STOP;
}
