#ifndef SENSORS_H
#define SENSORS_H


#include <stdbool.h>

// set status led
int set_status_led(uint16_t stat);

// initialize sensors (before state machine)
int sensors_init(void);

// read limit switch (can be polled from any thread)
// rturn 1 if the limit switch detects the object
bool read_limit_switch(void);

// check if button is currently pressed
bool button_pressed(void);

// get duration of last button press in milliseconds
int64_t get_button_press_duration(void);

// wait for a button press with timeout (call this while waiting in idle)
bool wait_for_button_press();


#endif