// heating_init() init function to configure the healing element gpio and thermistor adc 
// set_heating() function to toggle the heating element pin high or low
//      -> will need semaphore because of estop? But need to confirm estop will even interrupt that 
// read_thermistor_temp() function to read thermistor temperature
//      -> need to process the thermistor adc reading 
//      
// temp_safety_thread() starts a thread that reads from the adc -> prints the raw and processed value and checks if its too high
//      -> need to set Estop flag


#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/adc.h>

#include "heating.h"

#define ADC_NODE DT_NODELABEL(adc1)
#define TEMPTHREAD_STACK_SIZE 512
#define TEMPTHREAD_PRIORITY 2


// GPIO for heating
static const struct gpio_dt_spec heating_spec = GPIO_DT_SPEC_GET(DT_NODELABEL(heating_output), gpios);
static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
// Buffer for ADC sample
static uint16_t sample_buffer[1];
//will be used by the state machine
static volatile float current_temp = 0.0f; 

//atomic guarantees reading and writing happen in the same instruction
static atomic_t estop_flag = ATOMIC_INIT(0);


void set_estop_flag(void)
{
    atomic_set(&estop_flag, 1);

}

void clear_estop_flag(void)
{
    atomic_set(&estop_flag, 0);

}

bool check_estop_flag(void)
{
    return (atomic_get(&estop_flag) != 0);
}

//Thread must accept three void * args to match K_THREAD_DEFINE 
// starts temp safety thread after the adc is initialized
void temp_safety_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    // Waits for ADC to be ready (set up in heating_init)
   
    while (!adc_is_ready_dt(&adc_channel)) {
        k_msleep(100);
    }



    while (1) {
        // cast the return to a float
        current_temp = read_thermistor_temp();
        if (current_temp > MAX_SAFE_TEMP) {
            gpio_pin_set_dt(&heating_spec, 0);
            set_estop_flag();
        }
        k_msleep(100);
    }
}

K_THREAD_DEFINE(temp_thread_id, TEMPTHREAD_STACK_SIZE,
                temp_safety_thread, NULL, NULL, NULL,
                TEMPTHREAD_PRIORITY, 0, 0);

// function to toggle the heating element pin high or low
int set_heating(uint8_t stat){
    int err;
    err = gpio_pin_set_dt(&heating_spec, stat);
    if (err < 0){
        printk("failed to set heating gpio %d\n", err);
        return -1;
    }
    return 0;
}

// function to read thermistor temperature from the adc
float read_thermistor_temp(void) { 
    int err;
    int32_t adc_value = 0;
    
    // Configure the sequence for reading
    // temporary stack allocated struct (needs fresh sequence structure every time you read from adc)
    struct adc_sequence sequence = {
        .buffer = sample_buffer,
        .buffer_size = sizeof(sample_buffer),
    };
    
    // Setup the sequence from the ADC spec (this initializes channels, resolution, etc.)
    // populates teh sequence struct with device tree settings (no hardware configuration)
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
    printk("Thermistorrrr ADC raw: %d, mV: %d\n", adc_value, mv_value);
    
    // TODO: Convert mV to temperature
    return 25.0f;  // Placeholder
}




float get_current_temp(void) {
    // no race conditions because a float is 32 bits so writing is an atomic operation
    return current_temp;
}

// init function to configure the healing element gpio and thermistor adc
int heating_init(void){
    //initialize GPIO and the thermistor ADC for heating 
    int err;
    
    // Check if heating GPIO is ready
    if (!gpio_is_ready_dt(&heating_spec)) {
        printk("Heating GPIO device not ready\n");
        return -1;
    }
    
    // Initialize heating output (starts OFF for safety)
    err = gpio_pin_configure_dt(&heating_spec, GPIO_OUTPUT_INACTIVE);
    if (err < 0) {
        printk("Failed to configure heating pin: %d\n", err);
        return err;
    }


    // Check if ADC is ready
    if (!adc_is_ready_dt(&adc_channel)) {
        printk("ADC device not ready\n");
        return -1;
    }
    
    // Configure the ADC channel
    err = adc_channel_setup_dt(&adc_channel);
    if (err < 0) {
        printk("Failed to setup ADC channel (%d)\n", err);
        return err;
    }

     clear_estop_flag();
     printk("All initialized (heater_init) \n");
     return 0; 
}
