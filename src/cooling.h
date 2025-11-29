#ifndef COOLING_H
#define COOLING_H

double read_tach_speed(void);
int cooling_init(void);
void tach_monitoring_thread();
void set_fan(void);



#endif
