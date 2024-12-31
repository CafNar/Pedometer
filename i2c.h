#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stddef.h>

// Địa chỉ các thanh ghi I2C
#define I2C0_BASE  0x40090000 
#define I2C0_CTRL  (I2C0_BASE + 0x00)   // Control register
#define I2C0_STATUS (I2C0_BASE + 0x18)  // Status register
#define I2C0_FIFO  (I2C0_BASE + 0x10)   // Data FIFO register
#define I2C0_DIV   (I2C0_BASE + 0x14)   // Divider register

// Các bit trong các thanh ghi
#define I2C_CTRL_EN (1 << 0)        // Enable I2C
#define I2C_CTRL_START (1 << 1)     // Start condition
#define I2C_CTRL_STOP (1 << 2)      // Stop condition
#define I2C_CTRL_ACK (1 << 3)       // ACK bit
#define I2C_STATUS_RX_READY (1 << 1) // FIFO data ready for reading

// Địa chỉ I2C của các thiết bị
#define SSD1306_ADDR 0x3C  // Địa chỉ I2C của SSD1306
#define MPU6050_ADDR 0x68  // Địa chỉ I2C của MPU6050

// Hàm khởi tạo giao tiếp I2C
void i2c_init_custom(void);

// Hàm gửi dữ liệu qua I2C
void i2c_write(uint8_t addr, uint8_t *data, size_t len);

// Hàm đọc dữ liệu từ I2C
void i2c_read(uint8_t addr, uint8_t *data, size_t len);

#endif // I2C_H
