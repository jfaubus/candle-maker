//THIS IS FOR OUR SENSORS -> encoder + thru beam + limit switch
//also need to change tach reading to gpio
// flat guage sensor and thermistor are just fancy resistors

//thru beam -> digital signal -> needs MCU pullup (they probs added on board but i think its fine to do both)

// Idk if we actually need this in a separate file

// Global semaphore - initialize with count 1 (motor can run)
K_SEM_DEFINE(motor_enable_sem, 1, 1);

void thru_beam_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    // Take the semaphore to block motor thread
    k_sem_take(&motor_enable_sem, K_NO_WAIT);
}


void read_strain_guage(){
    // return 1 if the strain guage detects weight


}

void read_limit_switch(){
    // return 1 if the limit switch detects the object
}


// potentially will have to control the motor completely separately 
void wick_motor_control_thread(void)
{
    while (1) {
        // blocks here when thru beam is triggered
        k_sem_take(&motor_enable_sem, K_FOREVER);
        
        // Send step command via SPI
        send_motor_step();
        
        // Immediately give it back so we can take it again next iteration
        k_sem_give(&motor_enable_sem);
        
        k_usleep(step_delay_us);
    }
}