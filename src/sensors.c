//THIS IS FOR OUR SENSORS -> limit switch + start button (might need an ISR)
// is limit switch active low

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>


// device tree references
static const struct gpio_dt_spec limit_switch = GPIO_DT_SPEC_GET(DT_NOALIAS(limit_sw), gpios);
static const struct gpio_dt_spec start_button = GPIO_DT_SPEC_GET(DT_NOALIAS(start_btn), gpios);
static const struct gpio_dt_spec status_led = GPIO_DT_SPEC_GET(DT_NOALIAS(status_led), gpios);

// set status led
int set_status_led(uint16_t stat){
    int err;
    err = gpio_pin_set_dt(&status_led, stat);
    if (err < 0){
        printk("failed to set heating gpio %d\n", err);
        return -1;
    }
    return 0;
}

// button state tracking struct
struct button_state_t {
    bool is_pressed;
    int64_t press_start_time;
    int64_t press_duration_ms;
};
static struct button_state_t button_state = {0};

// GPIO callback structure
static struct gpio_callback button_cb_data;

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
    
    // Configure through-beam GPIO as input
    if (!gpio_is_ready_dt(&thru_beam)) {
        printk("Through-beam GPIO not ready\n");
        return -1;
    }

    ret = gpio_pin_configure_dt(&status_led, GPIO_PULL_UP);
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

// wait for a button press with timeout (call this while waiting in idle)
bool wait_for_button_press(int32_t timeout_ms)
{
    while (!button_state.is_pressed) {
        k_msleep(10);
    }
    return true; //button pressed
} 
