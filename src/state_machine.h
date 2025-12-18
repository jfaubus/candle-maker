#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <zephyr/kernel.h> 

// defines the states (used in statemachine.c and display.c)
enum state {
    IDLE,
    INIT_CHECK,
    WAX_DISPENSE,
    WAIT_FOR_TEMP,
    HEATING,
    SCENT_DISPENSE,
    STIRRING,
    WICK_INSERT,
    COOLING,
    ESTOP,
    ENDSTATE,
    WASH_CYCLE
};

// Declaration for the message queue to the thread for the display(this will be defined in my state machine file)
// extern basically means "trust that this is defined somewhere"
extern struct k_msgq state_msgq;

#endif