#ifndef COOLING_H
#define COOLING_H

// need to include this in header files if you use fixed width int types in a function signature
// zephyr includes this automatically in c files
#include <stdint.h>

double read_tach_speed(void);
int cooling_init(void);
void tach_monitoring_thread();
int set_fan(uint8_t duty_percent);
int stop_fan();
void start_cooling(void);
void stop_cooling(void);



#endif
