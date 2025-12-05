/*
 * TODO:
 * encoder
 * processing thermistor and tach input
 * sending UI messages (add message queue put in between state transitions)
 * talk to nikki about what happens in estop state
 * motor driver  (testing)
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
#include "servos.h"
#include "sensors.h"
#include "motor_step.h"   


//has an encoder
#define SCENT_ID        MOTOR_1
#define SCENT_STEPS     3200
#define SCENT_SPEED_US  1000    // 1ms per step = 1000 steps/sec

#define WAX_ID          MOTOR_3
#define WAX_STEPS       3200
#define WAX_SPEED_US    2000    // 2ms per step = 500 steps/sec
 
#define STIR_ID         MOTOR_2
#define STIR_STEPS      3200
#define STIR_SPEED_US   1000    // 1ms per step = 1000 steps/sec

#define LEAD_SCREW_ID       MOTOR_4
#define LEAD_SCREW_STEPS    3200
#define LEAD_SCREW_SPEED_US 2000    // 2ms per step = 500 steps/sec





K_MSGQ_DEFINE(state_msgq, sizeof(enum state), 10, 4);


struct machine_state {
    enum state current;
    uint32_t encoder_counts;
    float current_temp;
    bool wash_cycle;
};


// need a handle_estop function
// need a button_pressed function that returns how long the function was pressed for (returns one of two values)
void main(void) {
    struct machine_state machine = {.current = IDLE};
    
    // temp safety monitoring thread starts in heating.c at startup

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

    err = drv8434s_init();
    if (err < 0) {
        printk("Failed to init motors: %d\n", err);
        return;
    }

    err = door_lock_init();
    if (err < 0) {
        printk("Failed to init motors: %d\n", err);
        return;
    }

    err = sensors_init();
    if (err < 0){
        printk("Failed to initialize the limit switch and/or the start button %d\n", err);
        return;
    }


    while (1) {
        if (check_estop_flag() && machine.current != ESTOP && machine.current != IDLE) {
            printk("E-stop flag detected! Transitioning to ESTOP state\n");
            machine.current = ESTOP;
        }

        // Simple state execution
        switch(machine.current) {
            case IDLE:
                // turns status LED on
                set_status_led_mode(LED_ON);
                //DISPLAY: IDLE STATE
                printk("Waiting for button press...\n");

                // wait for button press
                wait_for_button_press();
                // if press less than 2000ms
                if (get_button_press_duration() <= 2000) {
                    machine.current = INIT_CHECK;
                }
                // if pressed greater than 200ms
                else if (get_button_press_duration() > 2000) {
                    machine.current = WASH_CYCLE;
                }
                // turns status LED off
                set_status_led_mode(LED_OFF);
                break;
                
            case WASH_CYCLE:
                printk("Wash cycle starting...");
                //DISPLAY: WASH CYCLE
               
                // sets status LED to fast blink
                set_status_led_mode(LED_FAST_BLINK);
                //display instructions
                // wait for user to press the start button again
                wait_for_button_press();

               //motor_move(SCENT_ID, SCENT_STEPS, SCENT_SPEED);
                motor_move_steps(SCENT_ID, SCENT_STEPS, MOTOR_DIR_CW, SCENT_SPEED_US);
                
                machine.wash_cycle = 1;
                // turns status LED off
                set_status_led_mode(LED_OFF);

                machine.current = IDLE;
                break;
                
            case INIT_CHECK:
                //DISPLAY: INIT STATE
                printk("Checking sensors...\n");

                // sets status LED to slow blink
                set_status_led_mode(LED_SLOW_BLINK);
                //if door closed, lock door
                if (read_limit_switch()) {
                    printk("Door closed!\n");

                } 
                else {
                    printk("Door not closed\n");
                    // DISPLAY: PLEASE CLOSE DOOR
                    int timeout_count = 0;
                    while (!read_limit_switch()) {
                        k_msleep(500);
            
                    if (++timeout_count > 120) {  // 60 second timeout (120 * 500ms)
                        printk("Door timeout returning to IDLE to prevent infinite loop\n");
                        machine.current = IDLE;
                        goto next_iteration;
                        //cant just do break because wed just be breaking this while loop not the case
                    }
                }
                }
                // now that the door is closed, lock the door
                err = door_lock();
                    if (err < 0) {
                        printk("Failed to use lock door servo: %d\n", err);
                        machine.current = ESTOP;
                        break;
                    }
                // turns status LED off
                set_status_led_mode(LED_OFF);
                machine.current = HEATING;
                break;
                
            
            case HEATING:
                // DISPLAY: BEGIN HEATING
                err = set_heating(1);
                if (err < 0) {
                    printk("Failed to set heating pin: %d\n", err);
                    machine.current = ESTOP;
                    break;
                }
                machine.current = WAX_DISPENSE;
                 
                break;

            case WAX_DISPENSE:
                // DISPLAY: WAX DISPENSE
                printk("Dispensing wax...\n");
                err = motor_move(WAX_ID, WAX_STEPS, WAX_SPEED);
                if (err < 0) {
                    printk("Failed to move wax dispendsing motor: %d\n", err);
                    machine.current = ESTOP;
                    break;
                }

                machine.current = WAIT_FOR_TEMP;
                break;

            case WAIT_FOR_TEMP:
                machine.current_temp = get_current_temp();
                printk("Waiting for temp to reach target \n");
    
                if (machine.current_temp >= TARGET_TEMP) {
                // Turn off heater and move to next state
                set_heating(0);
                 machine.current = SCENT_DISPENSE;
                }
                // If temp not reached, state stays as WAIT_FOR_TEMP
                break;
                

            case SCENT_DISPENSE:
                // DISPLAY: SCENT DISPENSE
                printk("dispensing scent");
                if(machine.wash_cycle == 1){
                     err = motor_move(SCENT_ID, SCENT_STEPS + 3200, SCENT_SPEED);
                }
                else{
                     err = motor_move(SCENT_ID, SCENT_STEPS, SCENT_SPEED);
                }
                if (err < 0) {
                    printk("Failed to move scent dispensing motor: %d\n", err);
                    machine.current = ESTOP;
                    break;
                }
                machine.current = STIRRING;
                break;

            case STIRRING:
                // DISPLAY: STIRRING STATE
                printk("stirring the wax");
                // lowers stirring mechanism
                err = motor_move(LEAD_SCREW_ID, LEAD_SCREW_STEPS, LEAD_SCREW_SPEED);
                if (err < 0) {
                    printk("Failed to move lead screw motor: %d\n", err);
                    machine.current = ESTOP;
                    break;
                }
                // starts stirring mechanism
                err = motor_move(STIR_ID, STIR_STEPS, STIR_SPEED);
                if (err < 0) {
                    printk("Failed to move stirring motor: %d\n", err);
                    machine.current = ESTOP;
                    break;
                }


                //BRING STIRRING MECHANISM BACK UP
                err = motor_move(LEAD_SCREW_ID, -LEAD_SCREW_STEPS, LEAD_SCREW_SPEED);
                if (err < 0) {
                    printk("Failed to move lead screw motor: %d\n", err);
                    machine.current = ESTOP;
                    break;
                }
                machine.current = WICK_INSERT;
                break;



            case WICK_INSERT:
                // DISPLAY: WICK INSERT STATE
                printk("starting the wick insert");
                //turn off heating element
                err = set_heating(0);
                if (err < 0) {
                    printk("Failed to set heating pin: %d\n", err);
                    machine.current = ESTOP;
                    break;
                }
                // start fans here?
                start_cooling();
                //drop the wick 
                err = move_wick_servo();
                if (err < 0) {
                    printk("Failed to start wick servo: %d\n", err);
                    machine.current = ESTOP;
                    break;
                }
                
            
                machine.current = COOLING;
                break;
            

            case COOLING:
                printk("Starting cooling process");
                //DISPLAY: COOLING STATE

                // WHEN TEMP REACHED/X AMOUNT OF TIME PASSED**** NEED TO TEST
                k_msleep(30000); 
                // stop fan
                stop_cooling();

                machine.current = ENDSTATE;
                break;

                
            case ENDSTATE:
                    printk("Candle  complete\n");
                    // any error checking??
                    // DISPLAY: SUCESSS MESSAGE
                    err = door_unlock();
                        if (err < 0) {
                        printk("Failed to unlock door: %d\n", err);
                    }
                    // Reset this flag
                    machine.wash_cycle = false;
                    machine.current = IDLE;  // Return to idle
                    break;
                
            case ESTOP:
                // save any current state taht are important? Maybe save current state and then see if something needs to be done
                //          -main one i can think of is sachins motors just dont wnat it to over a hole



                 // Stop everything 
                 // NEED TO STOP SERVOS****************************************************
                stop_all_motors();
                set_heating(0);
                stop_cooling();
                // DISPLAY: EMERGENCY STOP- press button to reset
                
                printk("System stopped. Press button to reset.\n");
                wait_for_button_press();
                clear_estop_flag();
                
                // Reset and return to IDLE
                machine.wash_cycle = false;
                machine.current = IDLE;
                printk("System reset return to IDLE\n");
                break;
        }
        next_iteration:
            k_msleep(100);  // Runs state machine at 10Hz

        
    }
}
