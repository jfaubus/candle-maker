#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <zephyr/kernel.h> 

enum state {
    IDLE,
    INIT_CHECK,
    WAX_DISPENSE,
    HEATING,
    SCENT_DISPENSE,
    STIRRING,
    WICK_INSERT,
    COOLING,
    ESTOP,
    ENDSTATE,
    WASH_CYCLE
};

// Declaration for our message queue (this will be defined in my state machine file)
// extern basically means "trust that this is defined somewhere"
extern struct k_msgq state_msgq;

#endif