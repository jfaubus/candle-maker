/*
 * TODO:
 * motor driver daisy chain
 * encoder
 * sachins sensor
 * processing thermistor and tach input
 * limit switch
 * sending UI messages
 * 
 * This weekend: 
 *      -state diagram frame (at least confirm each state is being entered using print messages and you can 
 *          simulate the encoder confirming x amount of turns by just starting the state and doing whatever you need to 
 *          do to get to the state after)
 *          -can you simulate estop? talk to nikki about what happens in estop state
 *      -motor driver daisy chain (test)
 *      -encoder
 *      -if you get the chance: limit swicth and sachins sensor
 * 
 *      
*/


/*
A state is represented by three functions, where one function implements the Entry 
actions, another function implements the Run actions, and the last function 
implements the Exit actions. The prototype for the entry and exit functions 
are as follows: void funct(void *obj), and the prototype for the run action 
is enum smf_state_result funct(void *obj) where the obj parameter is a user 
defined structure that has the state machine context, smf_ctx, as its first 
member. For example:

struct user_object {
   struct smf_ctx ctx;
   //All User Defined Data Follows 
};

*/

// Initial state = waiting for button press
/* 
 * 
 * s0
 * entry = button pressed for < 3 seconds
 * run =
 *          *if: strain guage detect(ed) weight -> all good Else: Display error message
 *          *if: limit switch detct(ed) the user closed the door -> all good, Else: display error message
 *          *return motors to original position>>> how do you determine what original position is?
 *  Exit = 3 all goods: -> go to state 2
 * 
 * s1
 * entry = button pressed for > 5 seconds
 * run = wash cycle 
 *          -turn scent motor x amount of times
 *          -make note that a wash cycle happened so s4 knows to do more spins
 * Exit = motor has turned x amount of times
 * 
 * s2
 * entry = s0 returned all good
 * run = wax dispensing motors
 * exit = motor turned x amount of times -> go to state 3
 * 
 * s3
 * entry = s2 went well
 * run = heating
 *          -start heating (gpio high)
 *          -start thread for temp monitoring
 *                  -puts message queue if temp is reached
 *                  -estops if temp is too high
 *                  -need debouncing
 * 
 * exit = goal temp is reached (go to s4) or temp is too high (go to estop state)
 * 
 * s4
 * entry = s3 decided goal temp was reached
 * run = scent dispensing motors
 * exit = motor turned x amount of times -> go to state 5 or 
 *       temperature is too high -> e-stop (s8)
 * 
 * s5
 * entry = s4 went well
 * run = stirring motors 
 * exit = stirring motor turned x amount of times -> go to state 6 or
 *        temperature is too high -> e-stop (s8)
 * 
 * s6
 * entry = s5 went well
 * run = start insert wick motors 
 *          -sensor says wick dropped
 *          -stop motors
 *          -move motors x amount of times to ensure the hole is closed
 * exit = all processes happened or
 *        temperature is too high -> e-stop (s8)
 * 
 * s7 
 * entry = s6 went well
 * run = start cooling candles
 *          -start pwm
 *          -check tach controller every x amount of seconds (timer controlled adc interrupts?)
 *          -adjust pwm accordingly 
 *          -wait x amount of time
 *          -release door
 * exit: all processes completed successfully
 * 
 * 
 * s8: ESTOP
 * run = stop all threads/motors
 *       display error message
 *       "how do we retunr to the original state?"
 */

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

struct machine_state {
    enum state current;
    uint32_t encoder_counts;
    float current_temp;
    bool estop_flag;
    bool wash_cycle;
};


// need a handle_estop function
// need a button_pressed function that returns how long the function was pressed for (returns one of two values)
void main(void) {
    struct machine_state machine = {.current = IDLE};
    
    // Start temperature monitoring thread (simple)
    k_thread_create(..., temp_monitor_thread, ...);
    
    while (1) {
        // Check E-stop first
        if (machine.estop_flag) {
            handle_estop(&machine);
            continue;
        }
        
        // Simple state execution
        switch(machine.current) {
            case IDLE:
                printk("Waiting for button press...\n");
                if (button_pressed()== 2 ) {
                    machine.current = INIT_CHECK;
                }
                else if (button_pressed() == 3) {
                    machine.current = WASH_CYCLE;
                }
                break;
                
            case WASH_CYCLE:
                printk("Wash cycle starting...");
                run_motor_to_position(CHIP_SELECT, NUMBEROFTURNS);
                machine.wash_cycle = 1;
                machine.current = IDLE;
                break;
                
            case INIT_CHECK:
                //retunring motors to a position?????
                printk("Checking sensors...\n");
                if (check_strain_gauge() && check_limit_switch()) {
                    printk("Checks passed!\n");
                    machine.current = WAX_DISPENSE;
                } else {
                    printk("Check failed, returning to IDLE (for now but later this will send a message to the screen with the error and prompt them to press the button again\n");
                    machine.current = IDLE;
                }
                break;
                
            case WAX_DISPENSE:
                printk("Dispensing wax...\n");
                run_motor_to_position(CHIP_SELECT, NUMBEROFTURNS);
                machine.current = HEATING;
                break;
                
            case HEATING:
                gpio_pin_set(heating_element, 1);  // Turn on heat
                printk("Heating... Current: %.1f°C\n", machine.current_temp);
                
                //NEED TO DECIDE TARFET TEMP
                if (machine.current_temp >= TARGET_TEMP) {
                    gpio_pin_set(heating_element, 0);
                    machine.current = SCENT_DISPENSE;
                }
                break;

            case SCENT_DISPENSE:
                printk("dispensing scent");
                if(machine.wash_cycle == 1){
                    run_motor_to_position(CHIP_SELECT, NUMBEROFTURNS + HOW EVER MUCH HANNAH SAID);
                }
                else{
                    run_motor_to_position(CHIP_SELECT, NUMBEROFTURNS);
                }
                machine.current = STIRRING;
                break;

            case STIRRING:
                printk("stirring the wax");
                run_motor_to_position(CHIP_SELECT, NUMBEROFTURNS);
                machine.current = WICK_INSERT;
                break;

            case WICK_INSERT:
                printk("starting the wick insert");
                run_motor_to_position(CHIP_SELECT, NUMBEROFTURNS);
                //STOP WHEN SENSOR REACHES POSITION
                //MOVE THE MOTOR A LITTLE MORE
                machine.current = COOLING;
                break;
            

            case COOLING:
                printk("Starting cooling process");
                start_cooling();
                //WHEN TEMP REACHED/X AMOUNT OF TIME PASSED

                machine.current = ENDSTATE;
                break;

                
            case ENDSTATE:
                //whatever happens at the end
                
            case ESTOP:
                // save any current state taht are important
                //          -main one i can think of is sachins motors just dont wnat it to over a hole
                stop_all_motors();
                gpio_pin_set(heating_element, 0);
                printk("!!! E-STOP - System halted !!!\n");
                // Wait for manual reset
                break;
        }
        
        k_msleep(100);  // Run state machine at 10Hz
    }
}

// Simple temp monitoring thread
void temp_monitor_thread(void *p1, void *p2, void *p3) {
    struct machine_state *machine = (struct machine_state *)p1;
    
    while (1) {
        machine->current_temp = read_thermistor_adc();
        
        if (machine->current_temp > DANGER_TEMP) {
            machine->estop_flag = true;
        }
        
        k_msleep(500);
    }
}