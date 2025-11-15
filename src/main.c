#include <zephyr/kernel.h>
#include "daisy_chained.h"

static struct drv8434s_chain motor_chain;

void step_motor_simple(uint8_t device)
{
    const uint8_t base_ctrl3 = 0x36;  // SPI mode + 1/16 microstepping
    
    // Set STEP bit
    drv8434s_write_register(&motor_chain, device, DRV8434S_CTRL3, 
                             base_ctrl3 | (1 << 6));
    k_usleep(200);  // Step rate
}

int main(void)
{
    int ret;
    uint8_t value;
    
    printk("\n=== DRV8434S Daisy Chain Test ===\n\n");
    
    // Initialize
    ret = drv8434s_chain_init(&motor_chain);
    if (ret < 0) {
        printk("Init failed: %d\n", ret);
        return ret;
    }
    
    k_msleep(10);
    
    // Configure all 3 motors
    printk("\n--- Configuring Motors ---\n");
    
    for (int dev = 1; dev <= 3; dev++) {
        printk("\nConfiguring motor %d:\n", dev);
        
        // Set current limit (CTRL1)
        drv8434s_write_register(&motor_chain, dev, DRV8434S_CTRL1, 0x40);
        k_msleep(5);
        
        // Enable outputs (CTRL2)
        drv8434s_write_register(&motor_chain, dev, DRV8434S_CTRL2, 0x80);
        k_msleep(5);
        
        // Set SPI mode + microstepping (CTRL3)
        drv8434s_write_register(&motor_chain, dev, DRV8434S_CTRL3, 0x36);
        k_msleep(5);
    }
    
    // Read back configuration to verify
    printk("\n--- Reading Back Configuration ---\n");
    
    for (int dev = 1; dev <= 3; dev++) {
        printk("\nMotor %d:\n", dev);
        
        drv8434s_read_register(&motor_chain, dev, DRV8434S_CTRL1, &value);
        printk("  CTRL1: 0x%02X (should be 0x40)\n", value);
        
        drv8434s_read_register(&motor_chain, dev, DRV8434S_CTRL2, &value);
        printk("  CTRL2: 0x%02X (should be 0x80)\n", value);
        
        drv8434s_read_register(&motor_chain, dev, DRV8434S_CTRL3, &value);
        printk("  CTRL3: 0x%02X (should be 0x36)\n", value);
        
        drv8434s_read_register(&motor_chain, dev, DRV8434S_FAULT, &value);
        printk("  FAULT: 0x%02X\n", value);
    }
    
    // Test stepping motor 1
    printk("\n--- Testing Motor 1 (20 steps) ---\n");
    for (int i = 0; i < 20; i++) {
        step_motor_simple(1);
        printk(".");
        if ((i + 1) % 10 == 0) {
            printk(" %d\n", i + 1);
        }
    }
    printk("\n");
    
    // Test stepping motor 2
    printk("\n--- Testing Motor 2 (20 steps) ---\n");
    for (int i = 0; i < 20; i++) {
        step_motor_simple(2);
        printk(".");
        if ((i + 1) % 10 == 0) {
            printk(" %d\n", i + 1);
        }
    }
    printk("\n");
    
    // Test stepping motor 3
    printk("\n--- Testing Motor 3 (20 steps) ---\n");
    for (int i = 0; i < 20; i++) {
        step_motor_simple(3);
        printk(".");
        if ((i + 1) % 10 == 0) {
            printk(" %d\n", i + 1);
        }
    }
    printk("\n");
    
    printk("\n=== Test Complete ===\n");
    
    // Disable all motors
    printk("Disabling outputs...\n");
    for (int dev = 1; dev <= 3; dev++) {
        drv8434s_write_register(&motor_chain, dev, DRV8434S_CTRL2, 0x00);
    }
    
    while (1) {
        k_msleep(1000);
    }
    
    return 0;
}