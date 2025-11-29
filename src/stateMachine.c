/*
 * TODO:
 * motor driver daisy chain
 * encoder
 * sachins sensor
 * processing thermistor and tach input
 * limit switch
 * sending UI messages (add message queue put in between state transitions)
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
 *       "how do we retunr fto the original state?"
 */

 /* ~~~~~~~~MOTOR IDs~~~~~~~~~
  *  scent (HANNAH) = 1
  #  wax (NICK) = 2
  #  stirring (DEVEN) = 3
  # wick (SACHIN) = 4 -> special case
 
 */


#include "state_machine.h"
#include "heating.h"
#include "cooling.h"

 #define SCENT_ID 1
 #define SCENT_STEPS 3200
 #define SCENT_SPEED 5

 #define WAX_ID 2
 #define WAX_STEPS 3200
 #define WAX_SPEED 5
 
 #define STIR_ID 3
 #define STIR_STEPS 3200
 #define STIR_SPEED 5

 #define WICK_ID 4
 #define WICK_STEPS 3200
 #define WICK_SPEED 5


K_MSGQ_DEFINE(state_msgq, sizeof(enum state), 10, 4);

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
    
    int err;
    err = heating_init();
    if (err < 0) {
        printk("Failed to init heating: %d\n", err);
        return;
    }
    
    err = cooling_init();
    if (err < 0) {
        printk("Failed to init cooling: %d\n", err);
        return;
    }

    err = motors_init();
    if (err < 0) {
        printk("Failed to init motors: %d\n", err);
        return;
    }

    
    while (1) {
        // Check E-stop first
        if (machine.estop_flag) {
            //***************IDK WHAT TO DO ABOUT THIS FUNCTION... *************/
            handle_estop(&machine);
            continue;
        }
        
        // Simple state execution
        switch(machine.current) {
            case IDLE:
                printk("Waiting for button press...\n");
                //*************Button_pressed() < 2 sec ? 1 : 0 ******************/
                if (button_pressed()) {
                    machine.current = INIT_CHECK;
                }
                else if (!button_pressed()) {
                    machine.current = WASH_CYCLE;
                }
                break;
                
            case WASH_CYCLE:
                printk("Wash cycle starting...");
                // motor_move in daisy_chained.c
                motor_move(SCENT_ID, SCENT_STEPS, SCENT_SPEED);
                machine.wash_cycle = 1;
                machine.current = IDLE;
                break;
                
            case INIT_CHECK:
                printk("Checking sensors...\n");
                //*****************NEED TO MAKE THESE FUNCTIONS *********/
                if (read_strain_gauge() && read_limit_switch()) {
                    printk("Checks passed!\n");
                    machine.current = WAX_DISPENSE;
                } else {
                    printk("Check failed, returning to IDLE (for now but later this will send a message to the screen with the error and prompt them to press the button again\n");
                    machine.current = IDLE;
                }

                break;
                
            case WAX_DISPENSE:
                printk("Dispensing wax...\n");
                motor_move(WAX_ID, WAX_STEPS, WAX_SPEED);
                machine.current = HEATING;
                break;
                
            case HEATING:

            //call function in Heating to set this pin high
                { // need to put this in curly brackets so err is only in scope in this switch case
                int err;
                err = set_heating(1);
                if (err < 0) {
                printk("Failed to set heating pin: %d\n", err);
                return err;
                }
                }

                 // Read temp directly when needed
                machine.current_temp = get_current_temp();
                printk("Heating... Current: %.1f°C\n", machine.current_temp);
                //************NEED TO DECIDE TARFET TEMP (define in heater.h)-> talk to ty
                if (machine.current_temp >= TARGET_TEMP) {
                    //gpio_pin_set(heating_element, 0);
                    set_heating(0);
                    printk("Target temp reached is reached -Next state: scent\n");
                    machine.current = SCENT_DISPENSE;
                }
                break;

            case SCENT_DISPENSE:
                printk("dispensing scent");
                if(machine.wash_cycle == 1){
                     motor_move(SCENT_ID, SCENT_STEPS + 3200, SCENT_SPEED);
                }
                else{
                     motor_move(SCENT_ID, SCENT_STEPS, SCENT_SPEED);
                }
                machine.current = STIRRING;
                break;

            case STIRRING:
                printk("stirring the wax");
                motor_move(STIR_ID, STIR_STEPS, STIR_SPEED);
                machine.current = WICK_INSERT;
                break;

            case WICK_INSERT:
                printk("starting the wick insert");
                //*******SACHIN SPECIAL CASE **************************************/
                motor_move(WICK_ID, WICK_STEPS, WICK_SPEED);
                //STOP WHEN SENSOR REACHES POSITION
                //MOVE THE MOTOR A LITTLE MORE
                machine.current = COOLING;
                break;
            

            case COOLING:
                printk("Starting cooling process");

                // start fan
                start_cooling();
                // WHEN TEMP REACHED/X AMOUNT OF TIME PASSED**** NEED TO TEST
                k_msleep(30000); 
                // stop fan
                stop_cooling();

                machine.current = ENDSTATE;
                break;

                
            case ENDSTATE:
                    printk("Candle  complete\n");
                    // whatever happens at the end -> unlock doorx + success message
                    machine.current = IDLE;  // Return to idle
                    break;
                
            case ESTOP:
                // save any current state taht are important? Maybe save current state and then see if something needs to be done
                //          -main one i can think of is sachins motors just dont wnat it to over a hole
                stop_all_motors();
                set_heating(0);
                stop_cooling();
                printk("E-STOP - System STOP \n");
                // Wait for manual reset
                while(1) {
                    k_msleep(1000);
                }
                break;
        }
        
        k_msleep(100);  // Run state machine at 10Hz
    }
}
