#ifndef COOLING_H
#define COOLING_H

// need to include this in header files if you use fixed width int types in a function signature
// zephyr includes this automatically in c files
#include <stdint.h>


int cooling_init(void);

int set_fan1(uint8_t percent_duty);
int set_fan2(uint8_t percent_duty);
int set_both_fans(uint8_t percent_duty);
int stop_fans(void);


uint32_t read_tach1_rpm(void);
uint32_t read_tach2_rpm(void);


void start_cooling(void);
void stop_cooling(void);

void tach_monitoring_thread(void *p1, void *p2, void *p3);
#endif



