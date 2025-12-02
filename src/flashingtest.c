#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

static const struct gpio_dt_spec testing_gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(testing_output), gpios);

// toggle testing pin high or low
int set_pin(int stat){
    int err;
    err = gpio_pin_set_dt(&testing_gpio, stat);  // Changed to testing_gpio
    if (err < 0){
        printk("failed to set testing gpio %d\n", err);
        return -1;
    }
    return 0;
}

int main(void)
{
    int err;
    printk("Starting GPIO test\n");
    
    // Check if GPIO is ready
    if (!gpio_is_ready_dt(&testing_gpio)) {
        printk("Testing GPIO device not ready\n");
        return -1;
    }
    
    // Initialize output (starts OFF for safety)
    err = gpio_pin_configure_dt(&testing_gpio, GPIO_OUTPUT_INACTIVE);  // Changed to testing_gpio
    if (err < 0) {
        printk("Failed to configure testing pin: %d\n", err);
        return err;
    }
    
    printk("GPIO configured successfully, starting test pattern\n");
    
    // Test pattern: 3 short pulses to verify control

    err = set_pin(1);
    if (err < 0) return err;
        
    k_sleep(K_MSEC(1000));  // On for 1 second
        
    printk("Test %d: Turning GPIO low");
    
        
    k_sleep(K_MSEC(2000));  // Off for 2 seconds
    
    //err = set_pin(0);
    //if (err < 0) return err;

    printk("Test complete GPIO  OFF\n");
    
    // Stay in loop doing nothing (GPIO remains off)
    while (1) {
        k_sleep(K_SECONDS(10));
    }
    
    return 0;
}