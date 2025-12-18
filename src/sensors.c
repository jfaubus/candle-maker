// The name of this file is a little misleading because it was originally for sensors that later got removed
// This file is for the status LED thread(with different blinking modes), the interrupt for the start button
// with helper function to determine how long the button was presed and poling the limit switch
// see sensors.h for functionl signatures (has descriptions)

// limit switch active low

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "sensors.h"

// device tree references
static const struct gpio_dt_spec limit_switch = GPIO_DT_SPEC_GET(DT_ALIAS(limit_sw), gpios);
static const struct gpio_dt_spec start_button = GPIO_DT_SPEC_GET(DT_ALIAS(start_btn), gpios);
static const struct gpio_dt_spec status_led = GPIO_DT_SPEC_GET(DT_ALIAS(status_led), gpios);


static volatile led_mode_t current_led_mode = LED_OFF;

// button state tracking struct
struct button_state_t {
    bool is_pressed;
    int64_t press_start_time;
    int64_t press_duration_ms;
};
static struct button_state_t button_state = {0};

// GPIO callback structure
static struct gpio_callback button_cb_data;

// LED control thread
#define LED_THREAD_STACK_SIZE 512
#define LED_THREAD_PRIORITY 5



// LED control thread function
void led_control_thread(void *p1, void *p2, void *p3) {
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    bool led_state = false;
    
    while (1) {
        switch (current_led_mode) {
            case LED_OFF:
                gpio_pin_set_dt(&status_led, 0);
                k_msleep(100);  // Check mode every 100ms
                break;
                
            case LED_ON:
                gpio_pin_set_dt(&status_led, 1);
                k_msleep(100);  // Check mode every 100ms
                break;
                
            case LED_SLOW_BLINK:
                led_state = !led_state;
                gpio_pin_set_dt(&status_led, led_state);
                k_msleep(500);  // Toggle every 500ms (1 Hz)
                break;
                
            case LED_FAST_BLINK:
                led_state = !led_state;
                gpio_pin_set_dt(&status_led, led_state);
                k_msleep(100);  // Toggle every 100ms (5 Hz)
                break;
        }
    }
}

K_THREAD_DEFINE(led_thread_id, LED_THREAD_STACK_SIZE,
                led_control_thread, NULL, NULL, NULL,
                LED_THREAD_PRIORITY, 0, 0);

// function for state machine to set LED mode
void set_status_led_mode(led_mode_t mode) {
    current_led_mode = mode;
    printk("LED mode set to: %d\n", mode);
}




// ISR for button
void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    // reads current state of gpio pin
    int button_val = gpio_pin_get_dt(&start_button);
    
    if (button_val == 1) {  // button pressed -> rising edge
        button_state.is_pressed = true;
        button_state.press_start_time = k_uptime_get();
    } 
    else {  // button released  -> falling edge
        button_state.is_pressed = false;
        button_state.press_duration_ms = k_uptime_get() - button_state.press_start_time;
    }
}

// initialize sensors (before state machine)
int sensors_init(void)
{
    int ret;
    
    // configure limit switch (as input and pull-up)
    if (!gpio_is_ready_dt(&limit_switch)) {
        printk("Limit switch GPIO not ready\n");
        return -1;
    }
    
    ret = gpio_pin_configure_dt(&limit_switch, GPIO_INPUT | GPIO_PULL_UP);
    if (ret < 0) {
        printk("Failed to configure limit switch: %d\n", ret);
        return ret;
    }


    ret = gpio_pin_configure_dt(&status_led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0){
        printk("failed to configure led status led: %d\n", ret);
        return ret;
    }
    
    // configure start button with interrupt
    if (!gpio_is_ready_dt(&start_button)) {
        printk("Start button GPIO not ready\n");
        return -1;
    }
    
    ret = gpio_pin_configure_dt(&start_button, GPIO_INPUT | GPIO_PULL_UP);
    if (ret < 0) {
        printk("Failed to configure start button: %d\n", ret);
        return ret;
    }
    
    // set up button interrupt for both edges (press and release)
    ret = gpio_pin_interrupt_configure_dt(&start_button, GPIO_INT_EDGE_BOTH);
    if (ret < 0) {
        printk("Failed to configure button interrupt: %d\n", ret);
        return ret;
    }
    
    // initialize and add the callback
    gpio_init_callback(&button_cb_data, button_isr, BIT(start_button.pin));
    gpio_add_callback(start_button.port, &button_cb_data);
    
    return 0;
}

// read limit switch (can be polled from any thread)
// rturn 1 if the limit switch detects the object
bool read_limit_switch(void)
{
    int val = gpio_pin_get_dt(&limit_switch);
    
    // assuming active-low with pull-up -> 0 = triggered, 1 = not triggered
    return (val == 0);  // returns true if limit switch is pressed
}

// check if button is currently pressed
bool button_pressed(void)
{
    return button_state.is_pressed;
}

// get duration of last button press in milliseconds
int64_t get_button_press_duration(void)
{
    return button_state.press_duration_ms;
}

// wait for a button press (call this while waiting in idle)
bool wait_for_button_press()
{
    while (!button_state.is_pressed) {
        k_msleep(10);
    }
    return true; //button pressed
} 
