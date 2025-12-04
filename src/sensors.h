#ifndef SENSORS_H
#define SENSORS_H


#include <stdbool.h>
#include <stdint.h>

// LED blinking modes
typedef enum {
    LED_OFF,
    LED_ON,
    LED_SLOW_BLINK,  // 1 Hz (500ms on, 500ms off)
    LED_FAST_BLINK   // 5 Hz (100ms on, 100ms off)
} led_mode_t;


// initialize sensors (before state machine)
int sensors_init(void);

// sets the status led mode
void set_status_led_mode(led_mode_t mode);

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