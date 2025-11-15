// what does the tach reading mean?


#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>


// pwm node
static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

// Tach input GPIO
static const struct gpio_dt_spec tach_input = GPIO_DT_SPEC_GET(DT_ALIAS(tach_input), gpios);



int main(void)
{

    printk("Screen started\n");
    int flag;
    



    // Check if PWM device is ready 
    if (!device_is_ready(pwm_led0.dev)) {
        printk("Fan PWM device not ready\n");
        return -1;
    }
    
    // Check if tach GPIO is ready
    if (!gpio_is_ready_dt(&tach_input)) {
        printk("Tach input GPIO not ready\n");
        return -1;
    }
    
    // Configure tach as input (pull-up already set in device tree)
    flag = gpio_pin_configure_dt(&tach_input, GPIO_INPUT);
    if (flag < 0) {
        printk("Failed to configure tach GPIO: %d\n", flag);
        return flag;
    }
    
    printk("Fan controller initialized\n");
    
    // PWM settings - using your device tree default
    uint32_t period_us = 500;  // 500µs = 2kHz
    uint32_t duty_percent = 0;
    
    // Start with fan stopped
    flag = pwm_set_dt(&pwm_led0, PWM_USEC(period_us), 0);
    if (flag < 0) {
        printk("Failed to initialize PWM: %d\n", flag);
        return flag;
    }
    
    printk("Fan stopped (starting safely)\n");
     //k_usleep(5);  // 5 ms to be safe
    
    // Start fan at 50% duty cycle
    duty_percent = 100;
    uint32_t pulse_us = (period_us * duty_percent) / 100;
    flag = pwm_set_dt(&pwm_led0, PWM_USEC(period_us), PWM_USEC(pulse_us));
    if (flag < 0) {
        printk("Failed to start PWM: %d\n", flag);
        return flag;
    }
    
    printk("Fan started at %d%% duty cycle\n", duty_percent);
    
    // Main loop just reads the tach pin state
    while (1) {
        int tach_value = gpio_pin_get_dt(&tach_input);
        printk("Testing screen");
        
        if (tach_value < 0) {
            printk("Error reading tach pin: %d\n", tach_value);
        } else {
            printk("Duty: %d%%, Tach state: %d\n", duty_percent, tach_value);
        }
        
       k_msleep(500);
    }
    
    return 0;
}