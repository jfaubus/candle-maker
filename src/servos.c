// need to set up pwm for wick and door servo in device tree
// need to add break beam gpio in the device tree
// need to put in sachins motor special case with the ISR and message queue + extra rotations to cover the hole


#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include "servos.h"

// door servo PWM
static const struct pwm_dt_spec door_servo = PWM_DT_SPEC_GET(DT_ALIAS(door_servo));
// wick servo PWM
static const struct pwm_dt_spec wick_servo = PWM_DT_SPEC_GET(DT_ALIAS(wick_servo));
// thru beam GPIO
static const struct gpio_dt_spec thru_beam = GPIO_DT_SPEC_GET(DT_ALIAS(thru_beam), gpios);
static struct gpio_callback thru_beam_cb_data;

// sets up the message queue for the ISR 
K_MSGQ_DEFINE(wick_msgq, sizeof(uint8_t), 1, 1);


void thru_beam_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    // POST MESSAGE QUEUE TO STOP SERVO MOTOR
    uint8_t stop_msg = 1;
    // Post to message queue (non-blocking from ISR)
    k_msgq_put(&wick_msgq, &stop_msg, K_NO_WAIT);
    printk("Through-beam triggered!\n");
}


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




// Set up interrupt on falling edge (beam broken) 
// falling edge because break beam is active low
err = gpio_pin_interrupt_configure_dt(&thru_beam, GPIO_INT_EDGE_FALLING); 
if (err < 0) { 
    printk("Failed to configure through-beam interrupt: %d\n", err); 
    return err; } 
    // Initialize callback 
    // arg1: pointer to the gpio_callback struct, arg2: handler function, arg3: pin mask -> a bit mask for the relavant pin
    gpio_init_callback(&thru_beam_cb_data, thru_beam_isr, BIT(thru_beam.pin)); 
    // arg1: pointer to the device structure for the specific GPIO driver instance, arg2: callback -> pointer to the gpio_callback struct
    gpio_add_callback(thru_beam.dev, &thru_beam_cb_data);

}


int door_lock(void) {
    // servo: 20ms period (50Hz)
    // 1.0ms pulse = 0 degrees(locked?)
    uint32_t period_us = 20000;
    uint32_t pulse_us = 1000;  // pulse width -> adjust for the servo's unlocked position************************
    
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
    uint32_t period_us = 20000; // period
    uint32_t pulse_us = 2000;  // pulse width -> adjust for the servo's unlocked position************************
    
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
// will need a message queue? Continue until message posted?
int move_wick_servo(void){

    uint32_t period_us = 20000;  // 20ms period/50Hz
    // NEED TO KNOW DIRECTION (adjust pulse_us: >1500 or <1500)*********************************************
    uint32_t pulse_us = 1500;     // Adjust this for speed (1500-2000)
    
    uint8_t msg;
    
    // Purge any old messages in the queue
    while (k_msgq_get(&wick_msgq, &msg, K_NO_WAIT) == 0) {
        // Empty the queue
        // ensures the servo starts in the first place
    }
    
    printk("Start wick servo rotation\n");
    
    // Start continuous rotation
    int err = pwm_set_dt(&wick_servo, PWM_USEC(period_us), PWM_USEC(pulse_us));
    if (err < 0) {
        printk("Failed to start wick servo: %d\n", err);
        return err;
    }
    
    // Wait for through-beam trigger blocks to wait
    err = k_msgq_get(&wick_msgq, &msg, K_FOREVER);
    if (err == 0) {
        printk("Through-beam detected, doing extra rotations for the hole\n");
        
        // Continue for extra rotations to cover the hole
        k_msleep(500);  // THIS WILL CHANGE WITH TESTING*****************************************
        
        // Stop the servo (1.5ms pulse = stopped for continuous rotation)
        pulse_us = 1500;
        pwm_set_dt(&wick_servo, PWM_USEC(period_us), PWM_USEC(pulse_us));
        
        printk("Wick servo stopped\n");
        return 0;
    }
    
    return -1;
}



