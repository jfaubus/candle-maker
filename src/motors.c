
#include "motors.h"
// This file has all of the helper functions for stepping the motors
// The motors were stepped using SPI (which isnt recommended going forward)
// To step the motors once you have to write a bit in one of the motor drivers registers high 
// So, to continuosly step you have to constantly write the bit high (it goes low automatically)
// This means you need a thread for continuous motor movement and two mutexes to prevent race conditions
// One mutex is to prevent multiple writes/reads over spi and the other mutex is for changing the motor command queue



// Motor command queue (one command per motor)
struct motor_command motor_commands[4] = {0};

// Mutex for motor command access
K_MUTEX_DEFINE(motor_cmd_mutex);
// Mutex for SPI bus access
K_MUTEX_DEFINE(spi_mutex);

// Semaphore to wake up motor thread
K_SEM_DEFINE(motor_sem, 0, 1);



// Motor thread
K_THREAD_DEFINE(motor_thread_id, MOTOR_THREAD_STACK_SIZE,
                motor_thread_entry, NULL, NULL, NULL,
                MOTOR_THREAD_PRIORITY, 0, 0);

const struct device *spi_dev1;
const struct device *spi_dev2;
const struct device *spi_dev3;
const struct device *spi_dev4;


struct spi_config spi_cfg1;
struct spi_config spi_cfg2;
struct spi_config spi_cfg3;
struct spi_config spi_cfg4;




int drv8434s_init(void) {
    printk("Initializing DRV8434S drivers\n");
    
    // Gets the SPI bus device (not the individual driver nodes)
    const struct device *spi_bus = DEVICE_DT_GET(DT_NODELABEL(spi2));


    
    
    if (!device_is_ready(spi_bus)) {
        printk("ERROR: SPI2 bus not ready\n");
        return -1;
    }
    
    // All three drivers use the same SPI bus
    spi_dev1 = spi_bus;
    spi_dev2 = spi_bus;
    spi_dev3 = spi_bus;
    spi_dev4 = spi_bus;


    
    printk("SPI bus ready\n");

    // Configure SPI settings for driver 1
    spi_cfg1.frequency = 1000000;
    spi_cfg1.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA | SPI_OP_MODE_MASTER;
    spi_cfg1.slave = 0;
    spi_cfg1.cs = (struct spi_cs_control){
        .gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(spi2), cs_gpios, 0),
        .delay = 0,
    };

    // Configure SPI settings for driver 2
    spi_cfg2.frequency = 1000000;
    spi_cfg2.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA | SPI_OP_MODE_MASTER;
    spi_cfg2.slave = 1;
    spi_cfg2.cs = (struct spi_cs_control){
        .gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(spi2), cs_gpios, 1),
        .delay = 0,
    };

    // Configure SPI settings for driver 3
    spi_cfg3.frequency = 1000000;
    spi_cfg3.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA | SPI_OP_MODE_MASTER;
    spi_cfg3.slave = 2;
    spi_cfg3.cs = (struct spi_cs_control){
        .gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(spi2), cs_gpios, 2),
        .delay = 0,
    };

    // Configure SPI settings for driver 4
    spi_cfg4.frequency = 1000000;
    spi_cfg4.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA | SPI_OP_MODE_MASTER;
    spi_cfg4.slave = 3;
    spi_cfg4.cs = (struct spi_cs_control){
        .gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(spi2), cs_gpios, 3),
        .delay = 0,
    };

    // initializing semaphore that blocks state machine until the motor is done being moved
    for (int i = 0; i < 4; i++) {
    k_sem_init(&motor_commands[i].completion_sem, 0, 1);
    }

    printk("SPI configurations set\n");
    return 0;
}



// Write to a SPECIFIC driver
int drv8434s_write_reg(const struct device *spi_dev, const struct spi_config *spi_cfg,uint8_t reg, uint8_t value)
//value = bit to write, reg = register address
{

    /* DRV8434S SPI Frame (16 bits total):
            Bit 15:    Reserved (always 0)
            Bit 14:    W (Write bit) - 0=write, 1=read
            Bits 13-9: A (Address) - 5-bit register address
            Bit 8:     Reserved (always 0)
            Bits 7-0:  D (Data) - 8-bit data value
    */
    uint16_t frame = 0;
    // put 0 at bit 14 just to tell the driver that the register is being written to
    frame |= (0 << 14);          // Bit 14: W=0 for write
    // or the frame with the reg address and 1F (because it makese sure only the lower 5 bit are used) and then shift it nine bits to put it in bits 13-9
    frame |= ((reg & 0x1F) << 9); // Bits 13-9: 5-bit address
    // put the data to be transfered into the first 8 bits and and it with 0xFF because it ensures the value is only 8 bits
    frame |= (value & 0xFF);      // Bits 7-0: 8-bit data

    // Send as two bytes, MSB first
    uint8_t tx_buf[2];
    tx_buf[0] = (frame >> 8) & 0xFF;  // Upper byte & 0xFF ensures its only 8 bits
    tx_buf[1] = frame & 0xFF;          // Lower byte & 0xFF ensures its only 8 bits
    

    // zephyr spi buffer struct
    struct spi_buf tx_spi_buf = {
        .buf = tx_buf, //pointer to the data array
        .len = 2 // length of the data array (in bytes)
    };

    // zephyr set of spi buffers struct
    struct spi_buf_set tx_spi_buf_set = {
        .buffers = &tx_spi_buf, // pointer to the buffers
        .count = 1 // just 1 for now 
    };

    // spi write automatically pulls CS low based on .slave in spi_cfg
    // then transfers all bytes in the buffer
    // pull CS high when done
    int flag;
    flag = spi_write(spi_dev, spi_cfg, &tx_spi_buf_set);
    k_usleep(5);  // 5 µs to be safe

    return flag;
}


// Spi read function
int drv8434s_read_reg(const struct device *spi_dev, const struct spi_config *spi_cfg, uint8_t reg, uint8_t *value)
//value = bit to write, reg = register address
{
    uint8_t tx_buf[2];
    uint8_t rx_buf[2];
    
    // Build 16-bit frame with read bit set
    uint16_t frame = 0;
    frame |= (1 << 14);           // Bit 14: W=1 for READ
    frame |= ((reg & 0x1F) << 9); // Bits 13-9: 5-bit address of the register being read from

    
    tx_buf[0] = (frame >> 8) & 0xFF; //lower 8 bytes (0xFF ensures its only 8)
    tx_buf[1] = frame & 0xFF;


    struct spi_buf tx_spi_buf = {
        .buf = tx_buf,
        .len = 2
    };
    struct spi_buf rx_spi_buf = {
        .buf = rx_buf,
        .len = 2
    };
    struct spi_buf_set tx_spi_buf_set = {
        .buffers = &tx_spi_buf,
        .count = 1
    };
    struct spi_buf_set rx_spi_buf_set = {
        .buffers = &rx_spi_buf,
        .count = 1
    };
    

    int flag = spi_transceive(spi_dev, spi_cfg, &tx_spi_buf_set, &rx_spi_buf_set);
    if (flag < 0) {
        printk("failed to read %d\n", flag);
        }

    k_usleep(5);  // 5 microseconds


    *value = rx_buf[1];  // Register data in lower byte

    return flag;
}




// Helper function to get the right SPI config for a motor
const struct device* get_motor_spi_dev(uint8_t motor_id) {
    switch(motor_id) {
        case 1: return spi_dev1;
        case 2: return spi_dev2;
        case 3: return spi_dev3;
        case 4: return spi_dev4;
        default: return NULL;
    }
}


struct spi_config* get_motor_spi_cfg(uint8_t motor_id) {
    switch(motor_id) {
        case 1: return &spi_cfg1;
        case 2: return &spi_cfg2;
        case 3: return &spi_cfg3;
        case 4: return &spi_cfg4;
        default: return NULL;
    }
}

// Initialize a motor for SPI control
int motor_init_for_spi_stepping(uint8_t motor_id) {
    const struct device *dev = get_motor_spi_dev(motor_id);
    struct spi_config *cfg = get_motor_spi_cfg(motor_id);
    
    if (!dev || !cfg) {
        printk("Invalid motor ID: %d\n", motor_id);
        return -1;
    }

    // Lock SPI mutex to ensure exclusive access to the SPI bus
    // K_FOREVER means wait indefinitely until mutex is available
    k_mutex_lock(&spi_mutex, K_FOREVER);
    

    // Configure CTRL1: Enable device, set decay mode

    // Bit 7-4: TRQ_DAC = 0000 (100% torque)
    drv8434s_write_reg(dev, cfg, DRV8434S_CTRL1_REG, 0x00);
    
    // Configure CTRL2: Microstepping mode
    // Bit 7: EN_OUT: 1 (enable)
    // Bit 2-0: 111 DECAY (try editing for smoother motion??)~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // 1000 0111 = 0x87
    drv8434s_write_reg(dev, cfg, DRV8434S_CTRL2_REG, 0x87);
    
    // Configure CTRL3: SPI stepping mode
    // Bit 7: DIR = 0 (forward, will change per command)
    // Bit 6: STEP = 0 (no step initially)
    // Bit 5: SPI_DIR =  1(spi controls dir)
    // Bits 3-0: MICROSTEP_MODE = 0000 (full step)
    // (0010 = 1/4 step, 0100 = 1/8 step, etc)
    // 0011 0000 = 0x30
    drv8434s_write_reg(dev, cfg, DRV8434S_CTRL3_REG, 0x30);
    
    k_mutex_unlock(&spi_mutex);
    
    printk("Motor %d initialized for SPI stepping\n", motor_id);
    return 0;
}
// Public function to queue a motor movement
int motor_move(uint8_t motor_id, int32_t steps, uint32_t speed_hz) {
    if (motor_id < 1 || motor_id > 4) {
        printk("Invalid motor ID: %d\n", motor_id);
        return -1;
    }
    
    if (speed_hz == 0 || speed_hz > 10000) {
        printk("Invalid speed: %d Hz (must be 1-10000)\n", speed_hz);
        return -1;
    }


    // Lock the motor command mutex to safely check/modify the command queue
    // K_FOREVER means wait indefinitely until we can acquire the lock
    k_mutex_lock(&motor_cmd_mutex, K_FOREVER);
    
    // Check if motor is already in use
    // subtract 1 because motor_id is 1-4, but array index is 0-3
    if (motor_commands[motor_id - 1].in_use) {
        // need to unlock before returning or the mutex stays locked forever (deadlock)
        k_mutex_unlock(&motor_cmd_mutex);
        printk("Motor %d is already moving\n", motor_id);
        return -EBUSY;
    }
    
    // If we get here, motor is available -> continue to queue the command
    // Set up the command structure
    motor_commands[motor_id - 1].motor_id = motor_id;
    motor_commands[motor_id - 1].steps = steps;
    motor_commands[motor_id - 1].speed_hz = speed_hz;
    motor_commands[motor_id - 1].in_use = true;
    
    // Now unlock the mutex since its done modifying shared data
    k_mutex_unlock(&motor_cmd_mutex);
    
    // Wake up motor thread to process new command
    k_sem_give(&motor_sem);
    
     printk("Motor %d: queued %d steps at %d Hz\n", motor_id, steps, speed_hz);
     printk("blocking until motors are finished moving");
    // blocks main state machine until motors are finished
    k_sem_take(&motor_commands[motor_id - 1].completion_sem, K_FOREVER);

   
    return 0;
}

// Motor control thread -> starts when motor_move is fully processed
void motor_thread_entry(void *p1, void *p2, void *p3) {
    printk("Motor thread started\n");
    
    while (1) {
        // Waits for a command
        k_sem_take(&motor_sem, K_FOREVER);
        
        // Process all pending motor commands
        for (int i = 0; i < 4; i++) {
            // gets the lock to motor_cmd_mutex so it cant be processed by motor_move at the same time
            k_mutex_lock(&motor_cmd_mutex, K_FOREVER);

            //if motors arent in use release the lock 
            if (!motor_commands[i].in_use) {
                k_mutex_unlock(&motor_cmd_mutex);
                continue;
            }
            
            // Copy command data safely 
            uint8_t motor_id = motor_commands[i].motor_id;
            int32_t steps = motor_commands[i].steps;
            uint32_t speed_hz = motor_commands[i].speed_hz;

            //once the motor_commands have been accessed + copied safely, release lock
            k_mutex_unlock(&motor_cmd_mutex);
            
            // Get device and config
            const struct device *dev = get_motor_spi_dev(motor_id);
            struct spi_config *cfg = get_motor_spi_cfg(motor_id);
            
            // Determine direction
            bool forward = (steps >= 0);
            uint32_t abs_steps = (steps >= 0) ? steps : -steps;
            uint32_t step_delay_us = 1000000 / speed_hz;
            
            // Calculate delay between steps (in microseconds)
            printk("Motor %d: moving %d steps %s at %d Hz (delay=%d us)\n",
                   motor_id, abs_steps, forward ? "forward" : "reverse", 
                   speed_hz, step_delay_us);

            // gets the lokc for using spi
            k_mutex_lock(&spi_mutex, K_FOREVER);
            
             // set direction in CTRL3
             // 1000 0000 = 0x80
            uint8_t ctrl3_val = 0x00;
            if (forward) {
                ctrl3_val |= 0x80;
            }
            drv8434s_write_reg(dev, cfg, DRV8434S_CTRL3_REG, ctrl3_val);
            
            // steppingggg loop
            for (uint32_t step = 0; step < abs_steps; step++) {
                drv8434s_write_reg(dev, cfg, DRV8434S_CTRL3_REG, ctrl3_val | 0x40);
                k_usleep(1);
                drv8434s_write_reg(dev, cfg, DRV8434S_CTRL3_REG, ctrl3_val);
                k_usleep(step_delay_us);
            }
            
            
             // releases lock for using spi
            k_mutex_unlock(&spi_mutex);
            
            //  marks command as complete (can be used for going to next state)
            k_mutex_lock(&motor_cmd_mutex, K_FOREVER);
            motor_commands[i].in_use = false;
            k_mutex_unlock(&motor_cmd_mutex);
            
            printk("Motor %d: movement complete\n", motor_id);
            // motors are done, free the state machine
            k_sem_give(&motor_commands[i].completion_sem);

            
        }
    }
}

// stops all motors -> for estop
void stop_all_motors(void) {
    k_mutex_lock(&motor_cmd_mutex, K_FOREVER);
    
    // clears all pending motor commands
    for (int i = 0; i < 4; i++) {
        motor_commands[i].in_use = false;
        // signal any blocked motor_move() calls to return
        k_sem_give(&motor_commands[i].completion_sem);
    }
    
    k_mutex_unlock(&motor_cmd_mutex);
    
    printk("All motors stopped (estop?)\n");
}