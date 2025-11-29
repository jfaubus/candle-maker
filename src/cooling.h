#ifndef COOLING_H
#define COOLING_H

double read_tach_speed(void);
int cooling_init(void);
void tach_monitoring_thread();
int set_fan(uint8_t duty_percent);

// Semaphore to wake up motor thread
K_SEM_DEFINE(cooling_sem, 0, 1);

#endif
