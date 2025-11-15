#ifndef DAISY_CHAIN_H
#define DAISY_CHAIN_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <string.h>

#define NUM_DEVICES 3
#define FRAME_SIZE 8

// Header byte definitions
#define HDR1_BASE 0x80
#define HDR2_BASE 0x80

// Register definitions
#define DRV8434S_FAULT  0x00
#define DRV8434S_DIAG1  0x01
#define DRV8434S_DIAG2  0x02
#define DRV8434S_CTRL1  0x03
#define DRV8434S_CTRL2  0x04
#define DRV8434S_CTRL3  0x05
#define DRV8434S_CTRL4  0x06
#define DRV8434S_CTRL5  0x07

struct drv8434s_chain {
    const struct device *spi_dev;
    struct spi_config spi_cfg;
};

// Just init, read, and write
int drv8434s_chain_init(struct drv8434s_chain *chain);
int drv8434s_write_register(struct drv8434s_chain *chain, uint8_t device_num, 
                             uint8_t reg_addr, uint8_t data);
int drv8434s_read_register(struct drv8434s_chain *chain, uint8_t device_num, 
                            uint8_t reg_addr, uint8_t *data);

#endif