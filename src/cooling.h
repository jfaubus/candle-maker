#ifndef COOLING_H
#define COOLING_H


// need to include this in header files if you use fixed width int types in a function signature
// zephyr includes this automatically in c files
#include <stdint.h>

// initializes GPIO pins, PWM signals
int cooling_init(void);
// sets pwm signal for fan 1
int set_fan1(uint8_t percent_duty);
// sets pwm signal for fan 2
int set_fan2(uint8_t percent_duty);
// sets pwm signal for both fans
int set_both_fans(uint8_t percent_duty);
// stops the fans (duty cycle -> 0)
int stop_fans(void);

// reads the tachs for each fan and resets pulse count
// helper function for the fan thread
uint32_t read_tach1_rpm(void);
uint32_t read_tach2_rpm(void);

// gives the semaphore to start the cooligng thread
void start_cooling(void);
// resets semaphore count to 0 (stops cooling thread)
void stop_cooling(void);


void tach_monitoring_thread(void *p1, void *p2, void *p3);
#endif



