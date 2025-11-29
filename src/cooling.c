// need to process tach reading
// need to add logic to monitor for big changes in tach and adjust pwm accordingly


#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/adc.h>


#define OPERATING_SPEED 80
#define SPEED_ERROR 10
#define TACHTHREAD_STACK_SIZE 512
#define TACHTHREAD_PRIORITY 2

// pwm node
static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

// Tach input GPIO
//static const struct gpio_dt_spec tach_input = GPIO_DT_SPEC_GET(DT_ALIAS(tach_input), gpios);

#define ADC_NODE DT_NODELABEL(adc1)

static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 3);

K_THREAD_DEFINE(tach_thread_id, TACHTHREAD_STACK_SIZE,
                tach_monitoring_thread, NULL, NULL, NULL, TACHTHREAD_PRIORITY, 0, 0);



// Buffer for ADC sample
static uint16_t sample_buffer[1];



int cooling_init(void){

    int flag;

    // Check if PWM device is ready 
    if (!device_is_ready(pwm_led0.dev)) {
        printk("Fan PWM device not ready\n");
        return -1;
    }

     printk("Fan controller initialized\n");

    // Check if ADC is ready
    if (!adc_is_ready_dt(&adc_channel)) {
        printk("ADC device not ready\n");
        return -1;
    }

    // Configure the ADC channel
    flag = adc_channel_setup_dt(&adc_channel);
    if (flag < 0) {
        printk("Failed to setup ADC channel (%d)\n", flag);
        return flag;
    }

    // PWM settings -> using device tree default
    uint32_t period_us = 500;  // 500µs = 2kHz
    uint32_t duty_percent = 0;
    
    // Start with fan stopped
    flag = pwm_set_dt(&pwm_led0, PWM_USEC(period_us), 0);
    if (flag < 0) {
        printk("Failed to initialize PWM: %d\n", flag);
        return flag;
    }
}



double read_tach_speed(void) { 
    int err;
    int32_t adc_value = 0;
    
    // Configure the sequence for reading
    struct adc_sequence sequence = {
        .buffer = sample_buffer,
        .buffer_size = sizeof(sample_buffer),
    };
    
    // Setup the sequence from the ADC spec (this initializes channels, resolution, etc.)
    err = adc_sequence_init_dt(&adc_channel, &sequence);
    if (err < 0) {
        printk("Failed to init ADC sequence (%d)\n", err);
        return -1.0;
    }
    
    // Read the ADC
    err = adc_read(adc_channel.dev, &sequence);
    if (err < 0) {
        printk("Could not read ADC (%d)\n", err);
        return -1.0;
    }
    
    // Get the raw ADC value
    adc_value = sample_buffer[0];
    
    // Convert raw ADC to millivolts
    int32_t mv_value = adc_value;
    err = adc_raw_to_millivolts_dt(&adc_channel, &mv_value);
    if (err < 0) {
        printk("Failed to convert to mV (%d)\n", err);
        return -1.0;
    }
    

    // print the raw adc value and the processed ADC value 
    printk("Tach ADC raw: %d, mV: %d\n", adc_value, mv_value);
    
    // TODO: Convert mV to speed
    return 25.0;  // Placeholder
}




//Thread must accept three void * args to match K_THREAD_DEFINE 
void tach_monitoring_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);


    while (1) {
        double speed = read_tach_speed();
        if (speed > OPERATING_SPEED + SPEED_ERROR) {
            //set pwm accordingly
            printk("Speed too high: %.1f\n", speed);
        }
        else if (speed < OPERATING_SPEED - SPEED_ERROR){
            printk("Speed too low: %.1f\n", speed);
        }
        k_msleep(100);
    }
}



void set_fan(int stat){
    // Start fan at 50% duty cycle
    if (stat == 1){
        duty_percent = 100;
        uint32_t pulse_us = (period_us * duty_percent) / 100;
        flag = pwm_set_dt(&pwm_led0, PWM_USEC(period_us), PWM_USEC(pulse_us));
        if (flag < 0) {
            printk("Failed to start PWM: %d\n", flag);
            return flag;
        }
        printk("Fan started at %d%% duty cycle\n", duty_percent);
    }
    // turn pwm off (fan off)
    else if (state == 0){
        flag = pwm_set_dt(&pwm_led0, PWM_USEC(period_us), 0);
        if (flag < 0) {
            printk("Failed to start PWM: %d\n", flag);
            return flag;
        }
        printk("turned off fan pwm");
    }
    else {
        printk("failed to start or stop the fan (invalid arg?)");
    }
}

