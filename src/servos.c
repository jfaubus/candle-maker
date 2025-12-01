// need to set up pwm for wick and door servo in device tree
// need to put in sachins motor special case with the ISR and message queue + extra rotations to cover the hole


#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include "door_lock.h"

// door servo PWM
static const struct pwm_dt_spec door_servo = PWM_DT_SPEC_GET(DT_ALIAS(door_servo));
// wick servo PWM
static const struct pwm_dt_spec wick_servo = PWM_DT_SPEC_GET(DT_ALIAS(wick_servo));

int door_lock_init(void) {
    if (!device_is_ready(door_servo.dev)) {
        printk("Door servo PWM not ready\n");
        return -1;
    }
    

    if (!device_is_ready(wick_servo.dev)) {
        printk("Door servo PWM not ready\n");
        return -1;
    }

    printk("Servos (for wick and door) initialized\n");
    return 0;
}


int door_lock(void) {
    // Hobby servo: 20ms period (50Hz)
    // 1.0ms pulse = 0 degrees(locked)
    uint32_t period_us = 20000;
    uint32_t pulse_us = 1000;  // Adjust for our servo's locked position
    
    int err = pwm_set_dt(&door_servo, PWM_USEC(period_us), PWM_USEC(pulse_us));
    if (err < 0) {
        printk("Failed to lock door: %d\n", err);
        return err;
    }
    
    printk("Door locked\n");
    k_msleep(500);  // Waits for servo to move
    return 0;
}

int door_unlock(void) {
    // 2.0ms pulse = 180 degrees (unlocked)
    uint32_t period_us = 20000;
    uint32_t pulse_us = 2000;  // Adjust for your servo's unlocked position
    
    int err = pwm_set_dt(&door_servo, PWM_USEC(period_us), PWM_USEC(pulse_us));
    if (err < 0) {
        printk("Failed to unlock door: %d\n", err);
        return err;
    }
    
    printk("Door unlocked\n");
    k_msleep(500);  // Wait for servo to move
    return 0;
}

// continously move the wick servo until the through beam ISR fires
// will need a semaphore or message queue? Continue until message posted?
int move_wick_servo(void){


}