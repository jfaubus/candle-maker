#include "motor_control.h"
#include <zephyr/drivers/gpio.h>



// Motor pin structure
typedef struct {
    const struct gpio_dt_spec step_pin;
    const struct gpio_dt_spec dir_pin;
} motor_pins_t;

// Defines all 4 motors
static const motor_pins_t motors[NUM_MOTORS] = {
    [MOTOR_1] = {
        .step_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(motor1_step_pin), gpios),
        .dir_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(motor1_dir_pin), gpios)
    },
    [MOTOR_2] = {
        .step_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(motor2_step_pin), gpios),
        .dir_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(motor2_dir_pin), gpios)
    },
    [MOTOR_3] = {
        .step_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(motor3_step_pin), gpios),
        .dir_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(motor3_dir_pin), gpios)
    },
    [MOTOR_4] = {
        .step_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(motor4_step_pin), gpios),
        .dir_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(motor4_dir_pin), gpios)
    }
};

int motor_init(void)
{
    int ret;

    for (int i = 0; i < NUM_MOTORS; i++) {
        // Configure STEP pin
        if (!gpio_is_ready_dt(&motors[i].step_pin)) {
            printk("Motor %d STEP GPIO not ready", i + 1);
            return -ENODEV;
        }
        ret = gpio_pin_configure_dt(&motors[i].step_pin, GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            printk("Failed to configure Motor %d STEP pin: %d", i + 1, ret);
            return ret;
        }

        // Configure DIR pin
        if (!gpio_is_ready_dt(&motors[i].dir_pin)) {
            printk("Motor %d DIR GPIO not ready", i + 1);
            return -ENODEV;
        }
        ret = gpio_pin_configure_dt(&motors[i].dir_pin, GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            printk("Failed to configure Motor %d DIR pin: %d", i + 1, ret);
            return ret;
        }

        printk("Motor %d initialized", i + 1);
    }

    // Give drivers time to power up
    k_msleep(1);

    printk("All motor drivers initialized successfully");
    return 0;
}

void motor_set_direction(motor_id_t motor, motor_direction_t dir)
{
    if (motor >= NUM_MOTORS) {
        printk("Invalid motor ID: %d", motor);
        return;
    }

    // Wait before changing direction
    k_busy_wait(1);
    gpio_pin_set_dt(&motors[motor].dir_pin, dir);
    k_busy_wait(1);
}

void motor_step(motor_id_t motor)
{
    if (motor >= NUM_MOTORS) {
        printk("Invalid motor ID: %d", motor);
        return;
    }

    // Minimum high pulse width is 970ns, we use 2us to be safe
    gpio_pin_set_dt(&motors[motor].step_pin, 1);
    k_busy_wait(2);
    gpio_pin_set_dt(&motors[motor].step_pin, 0);
    k_busy_wait(2);
}

void motor_move_steps(motor_id_t motor, uint32_t steps, motor_direction_t dir, uint32_t speed_us)
{
    if (motor >= NUM_MOTORS) {
        printk("Invalid motor ID: %d", motor);
        return;
    }

    motor_set_direction(motor, dir);
    
    for (uint32_t i = 0; i < steps; i++) {
        motor_step(motor);
        k_busy_wait(speed_us);
    }
}