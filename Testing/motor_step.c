// We decided last minute to switch to controlling the motor using step and direction pins
// So this was me trying to implement that
// This file is not tested because we didnt have working motor drivers

#include "motor_step.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>

// DRV8434S Register addresses
#define DRV8434S_REG_CTRL1    0x00
#define DRV8434S_REG_CTRL2    0x01
#define DRV8434S_REG_CTRL3    0x02

// SPI device
static const struct device *spi_dev;

// CS controls and SPI configs (initialized at runtime)
static struct spi_cs_control cs_ctrl[NUM_MOTORS];
static struct spi_config spi_cfg[NUM_MOTORS];

// GPIO pins for each motor
static const struct gpio_dt_spec motor_pins[NUM_MOTORS][2] = {
    [MOTOR_1] = {GPIO_DT_SPEC_GET(DT_NODELABEL(motor1_step_pin), gpios), GPIO_DT_SPEC_GET(DT_NODELABEL(motor1_dir_pin), gpios)},
    [MOTOR_2] = {GPIO_DT_SPEC_GET(DT_NODELABEL(motor2_step_pin), gpios), GPIO_DT_SPEC_GET(DT_NODELABEL(motor2_dir_pin), gpios)},
    [MOTOR_3] = { GPIO_DT_SPEC_GET(DT_NODELABEL(motor3_step_pin), gpios), GPIO_DT_SPEC_GET(DT_NODELABEL(motor3_dir_pin), gpios)},
    [MOTOR_4] = {GPIO_DT_SPEC_GET(DT_NODELABEL(motor4_step_pin), gpios), GPIO_DT_SPEC_GET(DT_NODELABEL(motor4_dir_pin), gpios)},
};

// CS GPIO specs
static const struct gpio_dt_spec cs_gpios[NUM_MOTORS] = {
    SPI_CS_GPIOS_DT_SPEC_GET(DT_NODELABEL(drv8434s_1)),
    SPI_CS_GPIOS_DT_SPEC_GET(DT_NODELABEL(drv8434s_2)),
    SPI_CS_GPIOS_DT_SPEC_GET(DT_NODELABEL(drv8434s_3)),
    SPI_CS_GPIOS_DT_SPEC_GET(DT_NODELABEL(drv8434s_4)),
};

// Write to DRV8434S register via SPI
static int drv8434s_write_reg(motor_id_t motor, uint8_t reg, uint8_t value)
{
    uint8_t tx_buf[2];
    tx_buf[0] = (reg << 1) | 0x00;
    tx_buf[1] = value;
    
    struct spi_buf tx_spi_buf = {.buf = tx_buf, .len = 2};
    struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};
    
    return spi_write(spi_dev, &spi_cfg[motor], &tx_spi_buf_set);
}

// Read from DRV8434S register via SPI
static int drv8434s_read_reg(motor_id_t motor, uint8_t reg, uint8_t *value)
{
    uint8_t tx_buf[2];
    uint8_t rx_buf[2] = {0};
    
    tx_buf[0] = (reg << 1) | 0x01;  // Bit 0 = 1 for read
    tx_buf[1] = 0x00;  // Dummy byte
    
    struct spi_buf tx_spi_buf = {.buf = tx_buf, .len = 2};
    struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};
    
    struct spi_buf rx_spi_buf = {.buf = rx_buf, .len = 2};
    struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_buf, .count = 1};
    
    int ret = spi_transceive(spi_dev, &spi_cfg[motor], &tx_spi_buf_set, &rx_spi_buf_set);
    if (ret == 0) {
        *value = rx_buf[1];  // Data is in second byte
    }
    return ret;
}

// Public wrapper
int motor_read_register(motor_id_t motor, uint8_t reg, uint8_t *value)
{
    return drv8434s_read_reg(motor, reg, value);
}
// Configure a single motor driver via SPI
static int configure_driver(motor_id_t motor)
{
    int ret;
    
    printk("Configuring motor %d driver via SPI...\n", motor + 1);
    
    // Reset driver (bit 0 = reset)
    ret = drv8434s_write_reg(motor, DRV8434S_REG_CTRL2, 0x01);
    if (ret < 0) {
        printk("Motor %d: Failed to reset: %d\n", motor + 1, ret);
        return ret;
    }
    k_msleep(1);
    
    // Set current limit to 50% (0x80)
    ret = drv8434s_write_reg(motor, DRV8434S_REG_CTRL1, 0x80);
    if (ret < 0) {
        printk("Motor %d: Failed to set current: %d\n", motor + 1, ret);
        return ret;
    }
    
    // Set full step mode
    ret = drv8434s_write_reg(motor, DRV8434S_REG_CTRL3, 0x00);
    if (ret < 0) {
        printk("Motor %d: Failed to set microstepping: %d\n", motor + 1, ret);
        return ret;
    }
    
    // Enable driver (bit 7 = EN_OUT = 1)
    ret = drv8434s_write_reg(motor, DRV8434S_REG_CTRL2, 0x80);  // Changed from 0x00!
    if (ret < 0) {
        printk("Motor %d: Failed to enable: %d\n", motor + 1, ret);
        return ret;
    }
    
    printk("Motor %d configured and enabled\n", motor + 1);
    return 0;
}

int motor_init(void)
{
    int ret;
    
    printk("Initializing motors...\n");
    
    // Get SPI device
    spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi2));
    if (!device_is_ready(spi_dev)) {
        printk("SPI device not ready\n");
        return -ENODEV;
    }
    
    // Initialize CS controls and SPI configs at runtime
    for (int i = 0; i < NUM_MOTORS; i++) {
        // Setup CS control
        cs_ctrl[i].gpio = cs_gpios[i];
        cs_ctrl[i].delay = 0;
        
        // Setup SPI config
        spi_cfg[i].frequency = 1000000;
        spi_cfg[i].operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER;
        spi_cfg[i].cs = cs_ctrl[i];  // Struct assignment
        
        // Configure CS GPIO
        if (!gpio_is_ready_dt(&cs_ctrl[i].gpio)) {
            printk("Motor %d CS GPIO not ready\n", i + 1);
            return -ENODEV;
        }
        ret = gpio_pin_configure_dt(&cs_ctrl[i].gpio, GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            printk("Motor %d: Failed to configure CS: %d\n", i + 1, ret);
            return ret;
        }
        
        // Configure STEP pin
        if (!gpio_is_ready_dt(&motor_pins[i][0])) {
            printk("Motor %d STEP GPIO not ready\n", i + 1);
            return -ENODEV;
        }
        ret = gpio_pin_configure_dt(&motor_pins[i][0], GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            printk("Motor %d: Failed to configure STEP: %d\n", i + 1, ret);
            return ret;
        }
        
        // Configure DIR pin
        if (!gpio_is_ready_dt(&motor_pins[i][1])) {
            printk("Motor %d DIR GPIO not ready\n", i + 1);
            return -ENODEV;
        }
        ret = gpio_pin_configure_dt(&motor_pins[i][1], GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            printk("Motor %d: Failed to configure DIR: %d\n", i + 1, ret);
            return ret;
        }
    }
    
    k_msleep(10);
    
    // Configure each driver via SPI
    for (int i = 0; i < NUM_MOTORS; i++) {
        ret = configure_driver(i);
        if (ret < 0) {
            return ret;
        }
    }
    
    printk("All motors initialized successfully\n");
    return 0;
}

void motor_set_direction(motor_id_t motor, motor_direction_t dir)
{
    if (motor >= NUM_MOTORS) {
        printk("Invalid motor ID: %d\n", motor);
        return;
    }
    
    k_busy_wait(1);
    gpio_pin_set_dt(&motor_pins[motor][1], dir);
    k_busy_wait(1);
}

void motor_step(motor_id_t motor)
{
    if (motor >= NUM_MOTORS) {
        printk("Invalid motor ID: %d\n", motor);
        return;
    }
    
    gpio_pin_set_dt(&motor_pins[motor][0], 1);
    k_busy_wait(2);
    gpio_pin_set_dt(&motor_pins[motor][0], 0);
    k_busy_wait(2);
}

void motor_move_steps(motor_id_t motor, uint32_t steps, motor_direction_t dir, uint32_t speed_us)
{
    if (motor >= NUM_MOTORS) {
        printk("Invalid motor ID: %d\n", motor);
        return;
    }
    
    motor_set_direction(motor, dir);
    
    for (uint32_t i = 0; i < steps; i++) {
        motor_step(motor);
        k_busy_wait(speed_us);
    }
}

void stop_all_motors(void)
{
    printk("Stopping all motors\n");
    
    for (int i = 0; i < NUM_MOTORS; i++) {
        gpio_pin_set_dt(&motor_pins[i][0], 0);
        gpio_pin_set_dt(&motor_pins[i][1], 0);
    }
}