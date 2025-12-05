#ifndef HEATING_H
#define HEATING_H

#define MAX_SAFE_TEMP 120
#define TARGET_TEMP 80


int set_heating(uint8_t stat);
float read_thermistor_temp(void);
int heating_init(void);
float get_current_temp(void);

bool check_estop_flag();
void set_estop_flag();
void clear_estop_flag();
#endif