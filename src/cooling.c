#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>
#include "cooling.h"

#define TACHTHREAD_STACK_SIZE 1024
#define TACHTHREAD_PRIORITY 2

#define OPERATING_SPEED_RPM 2000  // Target RPM***************************************literally no idea
#define SPEED_ERROR_RPM 200       // Tolerance (might increase?)******************************
#define DUTY_STEP 5
#define TACH_SAMPLE_PERIOD_MS 1000  // Sample for 1 second
#define PULSES_PER_REV 2  // Most PC fans = 2 pulses/revolution apparently but I need to confirm***********************

// Semaphore to control cooling thread
K_SEM_DEFINE(cooling_sem, 0, 1);

// PWM for both fans
static const struct pwm_dt_spec pwm_fan1 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_fan1));
static const struct pwm_dt_spec pwm_fan2 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_fan2));

// GPIO for both tach inputs
static const struct gpio_dt_spec gpio_tach1 = GPIO_DT_SPEC_GET(DT_ALIAS(gpio_tach1), gpios);
static const struct gpio_dt_spec gpio_tach2 = GPIO_DT_SPEC_GET(DT_ALIAS(gpio_tach2), gpios);

// Tach pulse counters
static volatile uint32_t tach1_pulse_count = 0;
static volatile uint32_t tach2_pulse_count = 0;

// GPIO callbacks
static struct gpio_callback tach1_cb_data;
static struct gpio_callback tach2_cb_data;

// Current duty cycles
static uint32_t period_us = 500;  // 2kHz
static uint8_t current_duty_fan1 = 0;
static uint8_t current_duty_fan2 = 0;


// Thread definition
K_THREAD_DEFINE(tach_thread_id, TACHTHREAD_STACK_SIZE,
                tach_monitoring_thread, NULL, NULL, NULL, 
                TACHTHREAD_PRIORITY, 0, 0);


// tach1 interrupt handler
void tach1_interrupt_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
    tach1_pulse_count++;
}
// tach2 interrupt handler
void tach2_interrupt_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
    tach2_pulse_count++;
}

int set_fan1(uint8_t percent_duty)
{
    // Start fan at duty_percent * period
    uint32_t pulse_us = (period_us * percent_duty) / 100;
    int err = pwm_set_dt(&pwm_fan1, PWM_USEC(period_us), PWM_USEC(pulse_us));
    if (err < 0) {
        printk("Failed to set Fan 1 PWM: %d\n", err);
        return err;
    }
    current_duty_fan1 = percent_duty;
    return 0;
}

int set_fan2(uint8_t percent_duty)
{
    // Start fan at duty_percent * period
    uint32_t pulse_us = (period_us * percent_duty) / 100;
    int err = pwm_set_dt(&pwm_fan2, PWM_USEC(period_us), PWM_USEC(pulse_us));
    if (err < 0) {
        printk("Failed to set Fan 2 PWM: %d\n", err);
        return err;
    }
    current_duty_fan2 = percent_duty;
    return 0;
}

int set_both_fans(uint8_t percent_duty)
{
    int err1 = set_fan1(percent_duty);
    int err2 = set_fan2(percent_duty);
    
    if (err1 < 0 || err2 < 0) {
        return -1;
    }
    
    printk("Both fans set to %d%% duty cycle\n", percent_duty);
    return 0;
}

int stop_fans(void)
{
    int err1 = pwm_set_dt(&pwm_fan1, PWM_USEC(period_us), 0);
    int err2 = pwm_set_dt(&pwm_fan2, PWM_USEC(period_us), 0);
    
    if (err1 < 0 || err2 < 0) {
        printk("Failed to stop fans\n");
        return -1;
    }
    
    current_duty_fan1 = 0;
    current_duty_fan2 = 0;
    printk("Both fans stopped\n");
    return 0;
}

// tach reading functions
uint32_t read_tach1_rpm(void)
{
    // Reset counter
    tach1_pulse_count = 0;
    
    // Wait for sample period
    k_msleep(TACH_SAMPLE_PERIOD_MS);
    
    // Calculate RPM
    // RPM = (pulses / sample_time_seconds) / pulses_per_rev * 60
    uint32_t rpm = (tach1_pulse_count * 60) / PULSES_PER_REV;
    
    printk("Fan 1 - Pulses: %u, RPM: %u\n", tach1_pulse_count, rpm);
    return rpm;
}

uint32_t read_tach2_rpm(void)
{
    // Reset counter
    tach2_pulse_count = 0;
    
    // Wait for sample period
    k_msleep(TACH_SAMPLE_PERIOD_MS);
    
    // Calculate RPM
    uint32_t rpm = (tach2_pulse_count * 60) / PULSES_PER_REV;
    
    printk("Fan 2 - Pulses: %u, RPM: %u\n", tach2_pulse_count, rpm);
    return rpm;
}



// start cooling threads
void start_cooling(void)
{
    current_duty_fan1 = 50;
    current_duty_fan2 = 50;
    set_both_fans(50);
    k_sem_give(&cooling_sem);  // Wake up monitoring thread
    printk("Cooling started at 50%% duty\n");
}

// stop cooling thread
void stop_cooling(void)
{
    k_sem_reset(&cooling_sem);  // Stop monitoring
    stop_fans();
    printk("Cooling stopped\n");
}

// init cooling 
int cooling_init(void)
{
    int err;

    // Check PWM devices
    if (!device_is_ready(pwm_fan1.dev)) {
        printk("Fan 1 PWM device not ready\n");
        return -1;
    }
    
    if (!device_is_ready(pwm_fan2.dev)) {
        printk("Fan 2 PWM device not ready\n");
        return -1;
    }


    // Configure tach GPIO inputs
    if (!device_is_ready(gpio_tach1.port)) {
        printk("Tach 1 GPIO device not ready\n");
        return -1;
    }
    
    if (!device_is_ready(gpio_tach2.port)) {
        printk("Tach 2 GPIO device not ready\n");
        return -1;
    }

    // Configure tach 1 as input with interrupt on rising edge
    err = gpio_pin_configure_dt(&gpio_tach1, GPIO_INPUT);
    if (err < 0) {
        printk("Failed to configure tach 1 GPIO: %d\n", err);
        return err;
    }

    err = gpio_pin_interrupt_configure_dt(&gpio_tach1, GPIO_INT_EDGE_RISING);
    if (err < 0) {
        printk("Failed to configure tach 1 interrupt: %d\n", err);
        return err;
    }

    // Configure tach 2 as input with interrupt on rising edge
    err = gpio_pin_configure_dt(&gpio_tach2, GPIO_INPUT);
    if (err < 0) {
        printk("Failed to configure tach 2 GPIO: %d\n", err);
        return err;
    }

    err = gpio_pin_interrupt_configure_dt(&gpio_tach2, GPIO_INT_EDGE_RISING);
    if (err < 0) {
        printk("Failed to configure tach 2 interrupt: %d\n", err);
        return err;
    }

    // Setup GPIO callbacks for the tach
    gpio_init_callback(&tach1_cb_data, tach1_interrupt_handler, BIT(gpio_tach1.pin));
    gpio_add_callback(gpio_tach1.port, &tach1_cb_data);
    gpio_init_callback(&tach2_cb_data, tach2_interrupt_handler, BIT(gpio_tach2.pin));
    gpio_add_callback(gpio_tach2.port, &tach2_cb_data);

    // Start fans stopped
    err = stop_fans();
    if (err < 0) {
        return err;
    }

    printk("Cooling system initialized\n");
    return 0;
}

// tach monitoring thread
void tach_monitoring_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    printk("Tach monitoring thread started\n");

    while (1) {
        // wait for cooling to be enabled (by start_cooling)
        k_sem_take(&cooling_sem, K_FOREVER);

        // monitoring loop
        while (k_sem_count_get(&cooling_sem) > 0) {
            // read both tach speeds
            uint32_t rpm1 = read_tach1_rpm();
            uint32_t rpm2 = read_tach2_rpm();

            // adjust fan 1 based on tach reading
            if (rpm1 > OPERATING_SPEED_RPM + SPEED_ERROR_RPM) {
                current_duty_fan1 = (current_duty_fan1 > DUTY_STEP) ? current_duty_fan1 - DUTY_STEP : 0;
                set_fan1(current_duty_fan1);
                printk("Fan 1 speed too high");
            } else if (rpm1 < OPERATING_SPEED_RPM - SPEED_ERROR_RPM) {
                current_duty_fan1 = (current_duty_fan1 < 100 - DUTY_STEP) ? current_duty_fan1 + DUTY_STEP : 100;
                set_fan1(current_duty_fan1);
                printk("Fan 1 speed too low");
            }

            // adjust fan 2 based on tach 2 reading
            if (rpm2 > OPERATING_SPEED_RPM + SPEED_ERROR_RPM) {
                current_duty_fan2 = (current_duty_fan2 > DUTY_STEP) ? current_duty_fan2 - DUTY_STEP : 0;
                set_fan2(current_duty_fan2);
                printk("Fan 2 speed too high");
            } else if (rpm2 < OPERATING_SPEED_RPM - SPEED_ERROR_RPM) {
                current_duty_fan2 = (current_duty_fan2 < 100 - DUTY_STEP) ? current_duty_fan2 + DUTY_STEP : 100;
                set_fan2(current_duty_fan2);
                printk("Fan 2 speed too low");
            }
            k_msleep(500);  // Check every 500ms
        }
    }
}