#ifndef MOTOR_STEP_H
#define MOTOR_STEP_H

#include <zephyr/kernel.h>

typedef enum {
    MOTOR_DIR_CW = 0,
    MOTOR_DIR_CCW = 1
} motor_direction_t;

typedef enum {
    MOTOR_1 = 0,
    MOTOR_2 = 1,
    MOTOR_3 = 2,
    MOTOR_4 = 3,
    NUM_MOTORS = 4
} motor_id_t;

// Initialize all motors
int motor_init(void);

// Control specific motor
void motor_set_direction(motor_id_t motor, motor_direction_t dir);
void motor_step(motor_id_t motor);
void motor_move_steps(motor_id_t motor, uint32_t steps, motor_direction_t dir, uint32_t speed_us);

#endif