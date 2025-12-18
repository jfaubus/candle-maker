// this was a test program to test if we could flash to the chip (sets a GPIO pin high)

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

static const struct gpio_dt_spec step_gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(motor1_step_pin), gpios);

void main(void)
{
    printk("Testing pin PA15\n");
    gpio_pin_configure_dt(&step_gpio, GPIO_OUTPUT_LOW);
    
    while (1) {
        gpio_pin_set_dt(&step_gpio, 1);
        k_msleep(1000);
        gpio_pin_set_dt(&step_gpio, 0);
        k_msleep(1000);
    }
}