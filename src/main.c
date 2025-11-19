// main.c
#include "daisy_chain.h"
// this is literally just for testing the motors, were going to see if i can get them to be smoother


int configure_driver(const struct device *spi_dev, 
                     const struct spi_config *spi_cfg,
                     int driver_num)
{
    int ret;
    printk("Configuring Driver %d \n", driver_num);
    
    k_mutex_lock(&spi_mutex, K_FOREVER);
    
    // CTRL2: Enable driver 
    ret = drv8434s_write_reg(spi_dev, spi_cfg, DRV8434S_CTRL2_REG, 0x8F);
    if (ret != 0) {
        printk("Driver %d: Failed to write CTRL2\n", driver_num);
        k_mutex_unlock(&spi_mutex);
        return ret;
    }


    

    
    // CTRL3: Enable SPI control + set microstepping
    // Bit 5: SPI_DIR = 1 (control direction via SPI)
    // Bit 4: SPI_STEP = 1 (control stepping via SPI)
    // Bits 3-0: 0110b = 1/16 microstepping
    uint8_t ctrl3_val = (1 << 5) |  // SPI_DIR enable
                        (1 << 4) |  // SPI_STEP enable
                        0x06;       // 1/16 microstepping
    
    ret = drv8434s_write_reg(spi_dev, spi_cfg, DRV8434S_CTRL3_REG, ctrl3_val);
    if (ret != 0) {
        printk("Driver %d: Failed to write CTRL3\n", driver_num);
        k_mutex_unlock(&spi_mutex);
        return ret;
    }
    
    // CTRL6 -> Set current limit to 1.5A
    ret = drv8434s_write_reg(spi_dev, spi_cfg, DRV8434S_CTRL6_REG, 153);
    if (ret != 0) {
        printk("Driver %d: Failed to write CTRL6\n", driver_num);
        k_mutex_unlock(&spi_mutex);
        return ret;
    }
    
    k_mutex_unlock(&spi_mutex);
    
    printk("Driver %d configured: SPI control enabled, 1/16 microstep, 1.5A\n", driver_num);
    return 0;
}

void test_read_driver(const struct device *spi_dev,
                      const struct spi_config *spi_cfg,
                      int driver_num)
{
    uint8_t value;
    int ret;
    
    k_mutex_lock(&spi_mutex, K_FOREVER);
    ret = drv8434s_read_reg(spi_dev, spi_cfg, DRV8434S_CTRL3_REG, &value);
    k_mutex_unlock(&spi_mutex);
    
    if (ret == 0) {
        printk("Driver %d CTRL3 value: 0x%02X\n", driver_num, value);
    } else {
        printk("Driver %d read failed\n", driver_num);
    }
}

// Step one motor - STEP bit is self-clearing!
int drv8434s_step_motor(const struct device *spi_dev, 
                        const struct spi_config *spi_cfg,
                        bool direction,
                        int num_steps)
{
    int ret;
    
    printk("Stepping: direction=%d, steps=%d\n", direction, num_steps);
    
    k_mutex_lock(&spi_mutex, K_FOREVER);
    
    for (int i = 0; i < num_steps; i++) {
        // 0xF6 = forward, 0x76 = backward
        //uint8_t ctrl3_val = direction ? 0xF6 : 0x76;
        uint8_t ctrl3_val = 0x76;
        
        ret = drv8434s_write_reg(spi_dev, spi_cfg, DRV8434S_CTRL3_REG, ctrl3_val);
        if (ret != 0) {
            printk("Step %d failed! ret=%d\n", i, ret);
            k_mutex_unlock(&spi_mutex);
            return ret;
        }
        
        if (i % 50 == 0) {  // Print every 50 steps
            printk("Step %d...\n", i);
        }
        
        k_usleep(1);
    }
    
    k_mutex_unlock(&spi_mutex);
    printk("Stepping complete!\n");
    return 0;
}


int drv8434s_step_motor_smooth(const struct device *spi_dev, 
                               const struct spi_config *spi_cfg,
                               bool direction,
                               int num_steps)
{
    int ret;
    k_mutex_lock(&spi_mutex, K_FOREVER);
    
    int accel_steps = 50;  // Ramp up/down over 50 steps
    
    for (int i = 0; i < num_steps; i++) {
        uint8_t ctrl3_val = 0x74;  // 1/4 microstep values
        
        ret = drv8434s_write_reg(spi_dev, spi_cfg, DRV8434S_CTRL3_REG, ctrl3_val);
        if (ret != 0) {
            k_mutex_unlock(&spi_mutex);
            return ret;
        }
        
        // Variable delay for acceleration/deceleration (trying to get the motion less choppy)
        int delay;
        if (i < accel_steps) {
            // Accelerate: start slow, get faster
            delay = 500 - (i * 400 / accel_steps);
        } else if (i > num_steps - accel_steps) {
            // Decelerate: slow down
            int remaining = num_steps - i;
            delay = 500 - (remaining * 400 / accel_steps);
        } else {
            // Constant speed in the middle
            delay = 100;
        }
        
        k_usleep(delay);
    }
    
    k_mutex_unlock(&spi_mutex);
    return 0;
}

int main(void)
{
    int ret;
    
    printk("\n DRV8434S Parallel Configuration Test\n\n");
    
    // Initialize - this fills in spi_dev1/2/3 and spi_cfg1/2/3
    ret = drv8434s_init();
    if (ret != 0) {
        printk("FATAL: Initialization failed!\n");
        return ret;
    }
    
    k_msleep(100);
    
    // Configure all drivers
    printk("\n--- Configuring all drivers ---\n");
    configure_driver(spi_dev1, &spi_cfg1, 1);  
    configure_driver(spi_dev2, &spi_cfg2, 2);  
    configure_driver(spi_dev3, &spi_cfg3, 3);  
    

    
    k_msleep(100);
    // After configuration, read back CTRL3:
    uint8_t readback;
    drv8434s_read_reg(spi_dev2, &spi_cfg2, DRV8434S_CTRL3_REG, &readback);
    printk("Driver 2 CTRL3 readback: 0x%02X (expected 0x36)\n", readback);
    
    
    k_msleep(100);
    // Comment out read test - we know communication works!
    /*
    printk("\n--- Reading back from drivers ---\n");
    test_read_driver(spi_dev1, &spi_cfg1, 1);
    test_read_driver(spi_dev2, &spi_cfg2, 2);
    test_read_driver(spi_dev3, &spi_cfg3, 3);
    */
    
    printk("\n Configuration Complete \n");
    
    // Test motor movement
    printk("\nTesting Motor Movement\n");
    k_msleep(1000);
    
    printk("Motor 1: 200 steps forward\n");
    drv8434s_step_motor_smooth(spi_dev2, &spi_cfg2, true, 200);
    k_msleep(500);
    
    printk("Motor 1: 200 steps backward\n");
    drv8434s_step_motor_smooth(spi_dev2, &spi_cfg2, false, 200);
    k_msleep(500);
    
    printk("\n Motor Test Done\n");
    
    while (1) {
        k_msleep(1000);
    }
    
    return 0;
}