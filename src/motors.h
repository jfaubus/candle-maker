#ifndef MOTORS_H
#define MOTORS_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>


// DRV8434S Register Addresses (from datasheet)
#define DRV8434S_FAULT_REG    0x00  // Fault status register
#define DRV8434S_DIAG1_REG    0x01  // Diagnostics 1
#define DRV8434S_DIAG2_REG    0x02  // Diagnostics 2
#define DRV8434S_CTRL1_REG    0x03  // Control register 1
#define DRV8434S_CTRL2_REG    0x04  // Control register 2 (DIR, STEP, SPI_DIR, SPI_STEP, MICROSTEP_MODE)
#define DRV8434S_CTRL3_REG    0x05  // Control register 3 (overcurrent check)
#define DRV8434S_CTRL4_REG    0x06  // Control register 4 (stall detection)
#define DRV8434S_CTRL5_REG    0x07  // Control register 5
#define DRV8434S_CTRL6_REG    0x08  // Control register 6
#define DRV8434S_CTRL7_REG    0x09  // Control register 7


//extern = tells compiler "these variables exists somewhere TRUST"
extern const struct device *spi_dev1;
extern const struct device *spi_dev2;
extern const struct device *spi_dev3;

extern struct spi_config spi_cfg1;
extern struct spi_config spi_cfg2;
extern struct spi_config spi_cfg3;

extern struct k_mutex spi_mutex;

// Motor command structure
struct motor_command {
    uint8_t motor_id;      // 1, 2, or 3
    int32_t steps;         // Positive = forward, negative = reverse
    uint32_t speed_hz;     // Steps per second
    bool in_use;           // Flag to indicate if command is active
};


// Thread stack and priority definitions
#define MOTOR_THREAD_STACK_SIZE 2048
#define MOTOR_THREAD_PRIORITY 5

// Function declarations
int motor_move(uint8_t motor_id, int32_t steps, uint32_t speed_hz);
void motor_thread_entry(void *p1, void *p2, void *p3);
int drv8434s_init(void);
int drv8434s_write_reg(const struct device *spi_dev, 
                       const struct spi_config *spi_cfg,
                       uint8_t reg, 
                       uint8_t value);
int drv8434s_read_reg(const struct device *spi_dev,
                      const struct spi_config *spi_cfg,
                      uint8_t reg,
                      uint8_t *value);

#endif // MOTORS_H







