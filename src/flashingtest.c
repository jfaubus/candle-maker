#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

static const struct gpio_dt_spec testing_gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(testing_output), gpios);

void main(void)
{
    printk("Starting GPIO test\n");
    
    // Check if GPIO is ready
    if (!gpio_is_ready_dt(&testing_gpio)) {
        printk("Testing GPIO device not ready\n");
        return;
    }
    
    // Configure as output
    int err = gpio_pin_configure_dt(&testing_gpio, GPIO_OUTPUT_INACTIVE);
    if (err < 0) {
        printk("Failed to configure pin: %d\n", err);
        return;
    }
    
    // Set it HIGH
    gpio_pin_set_dt(&testing_gpio, 0);
    printk("GPIO set HIGH on PB9\n");
    
    // Done - just loop forever
    while (1) {
        k_msleep(100); 
    }
}