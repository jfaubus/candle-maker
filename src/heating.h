#ifndef HEATING_H
#define HEATING_H

#define MAX_SAFE_TEMP 120
#define MAX_OPER_TEMP 80


int set_heating(int stat);
double read_thermistor_temp(void);
int heating_init(void);

#endif