#ifndef HEATING_H
#define HEATING_H



#define MAX_SAFE_TEMP 120
#define TARGET_TEMP 80


// function to toggle the heating element pin high or low
int set_heating(uint8_t stat);
// function to read thermistor temperature from the adc -> doesnt actuall work rn because it was never tested/ I never knew the conversion
float read_thermistor_temp(void);
// init function to configure the healing element gpio and thermistor adc
int heating_init(void);
// helper function- returns current temp
float get_current_temp(void);

// helper functions for estop flag
// i attempted to use an atomic variable
// atomic guarantees reading and writing happen in the same instruction
// never tested
bool check_estop_flag();
void set_estop_flag();
void clear_estop_flag();
#endif