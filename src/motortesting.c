/*
 * Stirring Motor Test Program
 * Tests the lead screw (motor 4) and stirring motor (motor 2)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include "motors.h"


//has an encoder
 #define SCENT_ID 1
 #define SCENT_STEPS 3200
 #define SCENT_SPEED 5000    // 200 µs → 5,000 Hz


 #define WAX_ID 3
 #define WAX_STEPS 3200
 #define WAX_SPEED 2000      // 0.5 µs → 2,000,000 Hz (seems like typo? using 500µs = 2000 Hz)
 
 #define STIR_ID 2
 #define STIR_STEPS 3200
 #define STIR_SPEED 10000    // 60 µs would be 16,667 Hz, but your code limits to 10,000 Hz max


 #define LEAD_SCREW_ID 4
 #define LEAD_SCREW_STEPS 3200
 #define LEAD_SCREW_SPEED 10000  // 100 µs → 10,000 Hz



void test_lead_screw_down(void) {
    printk("\n--- Testing Lead Screw DOWN ---\n");
    printk("Moving %d steps at %d Hz...\n", LEAD_SCREW_STEPS, LEAD_SCREW_SPEED);
    
    int err = motor_move(LEAD_SCREW_ID, LEAD_SCREW_STEPS, LEAD_SCREW_SPEED);
    if (err < 0) {
        printk("ERROR: Failed to move lead screw down: %d\n", err);
    } else {
        printk("SUCCESS: Lead screw moved down\n");
    }
}

void wax_dispense_motor(void) {
    printk("\n--- Testing Wax dispensing ---\n");
    printk("Moving %d steps at %d Hz...\n", WAX_STEPS, WAX_SPEED);
    
    int err = motor_move(WAX_ID, WAX_STEPS, WAX_SPEED);
    if (err < 0) {
        printk("ERROR: Failed to move wax dispensing motor: %d\n", err);
    } else {
        printk("SUCCESS: Wax dispensing motor\n");
    }
}

void test_lead_screw_up(void) {
    printk("\n--- Testing Lead Screw UP ---\n");
    printk("Moving %d steps (negative) at %d Hz...\n", -LEAD_SCREW_STEPS, LEAD_SCREW_SPEED);
    
    int err = motor_move(LEAD_SCREW_ID, -LEAD_SCREW_STEPS, LEAD_SCREW_SPEED);
    if (err < 0) {
        printk("ERROR: Failed to move lead screw up: %d\n", err);
    } else {
        printk("SUCCESS: Lead screw moved up\n");
    }
}

void test_stirring(void) {
    printk("\n--- Testing Stirring Motor ---\n");
    printk("Stirring %d steps at %d Hz...\n", STIR_STEPS, STIR_SPEED);
    
    int err = motor_move(STIR_ID, STIR_STEPS, STIR_SPEED);
    if (err < 0) {
        printk("ERROR: Failed to run stirring motor: %d\n", err);
    } else {
        printk("SUCCESS: Stirring complete\n");
    }
}
/*
void test_full_sequence(void) {
    printk("\n========== FULL STIRRING SEQUENCE ==========\n");
    
    // Step 1: Lower stirring mechanism
    printk("\nStep 1/3: Lowering stirring mechanism...\n");
    int err = motor_move(LEAD_SCREW_ID, LEAD_SCREW_STEPS, LEAD_SCREW_SPEED);
    if (err < 0) {
        printk("ERROR: Failed at step 1 (lower): %d\n", err);
        return;
    }
    printk("Step 1 complete!\n");
    k_msleep(500);  // Brief pause between steps
    
    // Step 2: Stir
    printk("\nStep 2/3: Stirring...\n");
    err = motor_move(STIR_ID, STIR_STEPS, STIR_SPEED);
    if (err < 0) {
        printk("ERROR: Failed at step 2 (stir): %d\n", err);
        return;
    }
    printk("Step 2 complete!\n");
    k_msleep(500);
    
    // Step 3: Raise stirring mechanism
    printk("\nStep 3/3: Raising stirring mechanism...\n");
    err = motor_move(LEAD_SCREW_ID, -LEAD_SCREW_STEPS, LEAD_SCREW_SPEED);
    if (err < 0) {
        printk("ERROR: Failed at step 3 (raise): %d\n", err);
        return;
    }
    printk("Step 3 complete!\n");
    
    printk("\n========== SEQUENCE COMPLETE! ==========\n");
}
*/



void main(void) {
    printk("\n\n========================================\n");
    printk("    STIRRING MOTOR TEST PROGRAM\n");
    printk("========================================\n\n");
    
    // Initialize motor drivers
    printk("Initializing DRV8434S motor drivers...\n");
    int err = drv8434s_init();
    if (err < 0) {
        printk("FATAL ERROR: Failed to initialize motors: %d\n", err);
        printk("Cannot continue. Check connections and devicetree.\n");
        return;
    }
    printk("Motor drivers initialized successfully!\n");

   
    
    // Initialize specific motors for SPI control
    printk("\nInitializing lead screw motor (ID %d)...\n", LEAD_SCREW_ID);
    err = motor_init_for_spi_stepping(LEAD_SCREW_ID);
    if (err < 0) {
        printk("ERROR: Failed to init lead screw motor\n");
        return;
    }
    
    printk("Initializing stirring motor (ID %d)...\n", STIR_ID);
    err = motor_init_for_spi_stepping(STIR_ID);
    if (err < 0) {
        printk("ERROR: Failed to init stirring motor\n");
        return;
    }


     // Test SPI communication by reading a register
    uint8_t test_val;
    const struct device *dev = get_motor_spi_dev(SCENT_ID);
    struct spi_config *cfg = get_motor_spi_cfg(SCENT_ID);



    // Read CTRL1 register (should return 0x01 if we just wrote it)
    drv8434s_write_reg(dev, cfg, DRV8434S_CTRL3_REG, 0x80);
    drv8434s_read_reg(dev, cfg, DRV8434S_CTRL3_REG, &test_val);
    printk("CTRL1 readback: 0x%02X (should be 0x01)\n", test_val);  
    
    printk("\n*** All motors ready for testing! ***\n");
    k_msleep(1000);
    
    // Main test loop



        
            printk("\nstarting wax dispensing\n");
            //wax_dispense_motor();

              /*  
            printk("\nLead screw up (reverse direction)\n");
            test_lead_screw_up();
            */
               
            printk("\nStirring test\n");
            //test_stirring();
            

                


}