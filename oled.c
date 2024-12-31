#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "oled.h"
#define I2C_PORT  i2c0
#define SDA_PIN 12
#define SCL_PIN 13
#define OLED_ADDR 0x3C

const uint8_t font_5x7[][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x49, 0x4F, 0x30}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x01, 0x71, 0x09, 0x07}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x26, 0x49, 0x49, 0x49, 0x3E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x26, 0x49, 0x49, 0x49, 0x32}, // S
    {0x38, 0x54, 0x54, 0x54, 0x48}, // e
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x48, 0x54, 0x54, 0x54, 0x24}, // s
    {0x00, 0x08, 0x3C, 0x48, 0x08}, // t
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
};
// Gửi một byte lệnh tới oled
void oled_send_command(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(I2C_PORT, OLED_ADDR, buf, 2, false);
}

// Gửi một byte dữ liệu tới oled
void oled_send_data(uint8_t data) {
    uint8_t buf[2] = {0x40, data};
    i2c_write_blocking(I2C_PORT, OLED_ADDR, buf, 2, false);
}

// Khởi tạo oled
void oled_init() {
    uint8_t init_cmds[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };

    for (size_t i = 0; i < sizeof(init_cmds); i++) {
        oled_send_command(init_cmds[i]);
    }
}

// Xóa màn hình
void oled_clear() {
    for (uint8_t page = 0; page < 8; page++) {
        oled_send_command(0xB0 + page); 
        oled_send_command(0x00);     
        oled_send_command(0x10);      

        for (uint8_t col = 0; col < 128; col++) {
            oled_send_data(0x00);
        }
    }
}

// Hiển thị một ký tự (5x7 font)
void oled_draw_char(uint8_t page, uint8_t col, char c) {
    // Điều chỉnh giá trị ký tự để nằm trong khoảng từ 0 đến 4
    if (c >= '0' && c <= '9') c -= '0'; // Ký tự số từ 0 đến 9
    else if (c == 'S') c = 11;
    else if (c == 'e') c = 12;
    else if (c == 'p') c = 13;
    else if (c == 's') c = 14;
    else if (c == 't') c = 15;
    else if (c == 'a') c = 16;
    else if (c == 'r') c = 17;
    else if (c == 'o') c = 18;
    else c = 10; 

    oled_send_command(0xB0 + page);    // Thiết lập địa chỉ trang
    oled_send_command(0x00 + (col & 0x0F)); // Thiết lập địa chỉ cột thấp
    oled_send_command(0x10 + (col >> 4));  // Thiết lập địa chỉ cột cao

    for (uint8_t i = 0; i < 5; i++) {
        oled_send_data(font_5x7[c][i]); // Gửi dữ liệu ký tự
    }
    oled_send_data(0x00); // Gửi byte trống để tạo khoảng cách giữa các ký tự
}
// Hiển thị chuỗi
void oled_draw_string(uint8_t page, uint8_t col, const char *str) {
    while (*str) {
        oled_draw_char(page, col, *str++);
        col +=6;
        if (col >= 128) break; 
    }
}

void oled_draw_number(uint8_t page, uint8_t col, int num) {
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%d", num);
    oled_draw_string(page, col, buffer);
}

void update_display(int steps, bool counting) {
    oled_clear();
    if (counting) {
        oled_draw_string(0, 0, "Start");
    } else {
        oled_draw_string(0, 0, "Stop");
    }
    oled_draw_string(1, 0, "Steps:");
    oled_draw_number(2, 0, steps);
}