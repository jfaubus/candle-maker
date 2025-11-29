#ifndef COOLING_H
#define COOLING_H

double read_tach_speed(void);
int cooling_init(void);
void tach_monitoring_thread();
int set_fan(int duty_percent);

#endif
