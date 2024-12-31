#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "mpu.h"
#include "oled.h"

#define LED_RED     14
#define LED_GREEN   15
#define STATE_PIN   16
#define RESET_PIN   17
// Cấu hình I2C
#define I2C_PORT i2c0
#define I2C_SDA 12
#define I2C_SCL 13
// Base addresses từ datasheet RP2040
#define CLOCKS_BASE         0x40010000   // Clock controller base
#define RESETS_BASE         0x40020000   // Reset controller base  
#define IO_BANK0_BASE       0x40028000   // GPIO controller base
#define PADS_BANK0_BASE     0x40038000   // Pad control base
#define SIO_BASE            0xd0000000   // SIO base

// Clock registers
#define CLK_GPOUT0_CTRL     0x00         // Clock control register
#define CLK_GPOUT0_DIV      0x04         // Clock divider register
#define CLK_GPOUT0_SELECTED 0x08        // Selected clock source

#define RESETS_RESET_FRCE_ON    0x0    // Force on - clear reset
#define RESETS_RESET_FRCE_OFF   0x4    // Force off - set reset  
#define RESETS_RESET_WDSEL      0x8    // Watchdog reset select
#define RESETS_RESET_DONE       0xc    // Reset done status

// GPIO registers 
#define GPIO_OUT_SET        0x018        // Output set register
#define GPIO_OUT_CLR        0x020        // Output clear register  
#define GPIO_OE_SET         0x038        // Output enable set register
#define GPIO_OE_CLR         0x044        // Output enable clear register

#define GPIO_IN_REG         0x004        // Input value register
#define GPIO_CTRL_REG       0x004   // GPIO control register offset
#define GPIO_STATUS_REG     0x000   // GPIO status register offset

#define INTR_RAW_0         0x0A0   // Raw Interrupts 0
#define INTR_0             0x0B0   // Interrupt status after masking & forcing 0
#define INTR_ENABLE_0      0x0C0   // Interrupt enable 0
#define INTR_FORCE_0       0x0D0   // Interrupt force 0

#define SYSCFG_BASE         0x40008000
#define PROC0_NMI_MASK0      0x0

#define STEP_THRESHOLD 14000

static int steps = 0;       // Biến đếm số bước chân
static bool isCounting = false; // Chế độ hiện tại (true: nháy LED xanh, false: LED đỏ bật)
static bool prevStateButton = true;

static inline void reg_write(uintptr_t addr, uint32_t value) {
    *(volatile uint32_t*)addr = value;
}

static inline uint32_t reg_read(uintptr_t addr) {
    return *(volatile uint32_t*)addr;
}

void my_gpio_init(uint32_t pin) {
    // Reset state first
    reg_write(SIO_BASE + GPIO_OE_CLR, (1u << pin));
    reg_write(SIO_BASE + GPIO_OUT_CLR, (1u << pin));
              
    if (pin == LED_RED) {
        reg_write(IO_BANK0_BASE + 0x074, 5);
        reg_write(SIO_BASE + GPIO_OE_SET, (1u << pin));
        reg_write(PADS_BANK0_BASE + (pin * 4), 
                 (1 << 3) |       // Slew rate fast
                 (1 << 6) |       // Drive strength
                 (1 << 7));       // Output enable
    } 
    else if (pin == LED_GREEN) {
        reg_write(IO_BANK0_BASE + 0x07c, 5);
        reg_write(SIO_BASE + GPIO_OE_SET, (1u << pin));
        reg_write(PADS_BANK0_BASE + (pin * 4), 
                 (1 << 3) |       // Slew rate fast
                 (1 << 6) |       // Drive strength
                 (1 << 7));       // Output enable
    }
    else if (pin == STATE_PIN) {
        // Function select for GPIO16 - set to GPIO
        reg_write(IO_BANK0_BASE + 0x084, 5); 
        
        // Configure pad control for GPIO16:
        // Bit 3: PUE (Pull-up enable) = 1
        // Bit 2: PDE (Pull-down enable) = 0 
        // Bits 5:4: DRIVE = 01 (4mA)
        reg_write(PADS_BANK0_BASE + (pin * 4), 
                 (1 << 3) |    // Enable pull-up
                 (1 << 5));    // Set drive strength 4mA
    }
    else if (pin == RESET_PIN) {
        // Function select for GPIO17 - set to GPIO
        reg_write(IO_BANK0_BASE + 0x08c, 5); 
        
        // Configure pad control for GPIO17:
        // Bit 3: PUE (Pull-up enable) = 1
        // Bit 2: PDE (Pull-down enable) = 0 
        // Bits 5:4: DRIVE = 01 (4mA)
        reg_write(PADS_BANK0_BASE + (pin * 4), 
                 (1 << 3) |    // Enable pull-up
                 (1 << 5));    // Set drive strength 4mA
    }
}

void led_on(uint32_t pin) {
    // Explicitly clear previous state
    reg_write(SIO_BASE + GPIO_OUT_CLR, (1u << pin));
    reg_write(SIO_BASE + GPIO_OE_SET, (1u << pin));
    reg_write(SIO_BASE + GPIO_OUT_SET, (1u << pin));
}

void led_off(uint32_t pin) {
    // Explicitly disable output and clear output
    reg_write(SIO_BASE + GPIO_OUT_CLR, (1u << pin));
    reg_write(SIO_BASE + GPIO_OE_CLR, (1u << pin));
}

void delay(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 12000; i++) {
        __asm("nop");
    }
}


int main() {
    reg_write(RESETS_BASE + RESETS_RESET_FRCE_ON, 0);
    reg_write(RESETS_BASE + RESETS_RESET_FRCE_OFF, (1 << 5) | (1 << 6) | (1 << 8));
    while ((reg_read(RESETS_BASE + RESETS_RESET_DONE) & ((1 << 5) | (1 << 6) | (1 << 8))) == 0);

    my_gpio_init(LED_RED);
    my_gpio_init(LED_GREEN);
    my_gpio_init(STATE_PIN);
    my_gpio_init(RESET_PIN);

    i2c_init(I2C_PORT, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    oled_init();
    oled_clear();
    update_display(steps, isCounting);

    while (1) {
        uint32_t state = gpio_get(STATE_PIN);
        uint32_t reset_state = gpio_get(RESET_PIN);
        if (state == 0 && prevStateButton == 1) {
            isCounting = !isCounting;
            if (isCounting) {
                led_on(LED_GREEN);
                led_off(LED_RED);
            } else {
                led_on(LED_RED);
                led_off(LED_GREEN);
            }
        }
        prevStateButton = state;
    }
        
    return 0;
}