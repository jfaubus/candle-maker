#include "state_machine.h"
#include <lvgl.h>

void display_thread(void) {
    enum state current_state = IDLE;
    
    while(1) {
        // Check for new state (non-blocking)
        enum state new_state;
        // checks the message queue 
        //          &state_msgq = message queue address
        //          &new_state = address to save the data to (in our case this is the state)
        //          K_NO_WAIT means if there is no data to process then move on with the program (we dont want to block the rest of the program)
        if (k_msgq_get(&state_msgq, &new_state, K_NO_WAIT) == 0) {
            current_state = new_state;
            update_display_state(current_state);
        }
        
        // LVGl screen handling + applies any changes that are queued up (like update screen)
        lv_task_handler();
        // controls the update rate
        // (we dont want lv_task_handler() to be repeatedly called)
        k_msleep(20);
    }
}

void update_display_state(enum state state) {
    // checks the state and updates the display screen accordingly 
    switch(state) {
        case IDLE:
            // i think this is the lvgl function youll use, if not feel free to change
            // where idle_screen is the screen object that you'll define
            lv_scr_load(idle_screen);
            break;
            
        case INIT_CHECK:
            lv_scr_load(init_check_screen);
            break;
            
        case WAX_DISPENSE:
            lv_scr_load(wax_dispense_screen);
            break;
            
        case HEATING:
            lv_scr_load(heating_screen);
            break;
            
        case SCENT_DISPENSE:
            lv_scr_load(scent_dispense_screen);
            break;
            
        case STIRRING:
            lv_scr_load(stirring_screen);
            break;
            
        case WICK_INSERT:
            lv_scr_load(wick_insert_screen);
            break;
            
        case COOLING:
            lv_scr_load(cooling_screen);
            break;
            
        case ESTOP:
            lv_scr_load(estop_screen);
            break;
            
        case ENDSTATE:
            lv_scr_load(done_screen);
            break;
            
        case WASH_CYCLE:
            lv_scr_load(wash_cycle_screen);
            break;
            
        default:
            // just in case, will also use for debugging
            printk("Unknown state: %d\n", state);
            break;
    }
}