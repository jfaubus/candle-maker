============
Candle Maker
============

An automated candle-making machine built with embedded systems and real-time operating system principles.

Overview
========

This project automates the candle production process using a multi-threaded state machine running on Zephyr RTOS. The system coordinates stepper motors, heating control, cooling fans, and servos to produce candles from wax dispensing through cooling.

Hardware
========

* **Microcontroller:** STM32F446RE (ARM Cortex-M4)
* **Development Board:** Nucleo-F446RE
* **Motor Drivers:** 4× DRV8434S (SPI communication)
* **Motors:**
  
  - Wax dispenser stepper motor
  - Scent dispenser stepper motor
  - Stirrer motor with lead screw positioning
  - Wick insertion servo

* **Temperature Control:**
  
  - Heating element (SSR-controlled)
  - NTC thermistor (ADC monitoring)

* **Cooling System:**
  
  - 2× PWM-controlled fans with tachometer feedback

* **Sensors:**
  
  - Door limit switch
  - Through-beam sensor (wick detection)
  - Start button with duration sensing
  - Status LED

Software Architecture
=====================

State Machine
-------------

The system operates through 12 distinct states:

* **IDLE** - Waiting for button press (short = normal cycle, long = wash)
* **INIT_CHECK** - Door verification and locking
* **HEATING** - Activate heating element
* **WAX_DISPENSE** - Dispense wax into mold
* **WAIT_FOR_TEMP** - Poll until target temperature (80°C) reached
* **SCENT_DISPENSE** - Add scent to melted wax
* **STIRRING** - Mix wax and scent
* **WICK_INSERT** - Insert wick via servo (stops on through-beam trigger)
* **COOLING** - Cool candle with fans
* **ENDSTATE** - Unlock door, reset for next cycle
* **WASH_CYCLE** - Cleaning cycle for scent dispenser
* **ESTOP** - Emergency stop (triggered by overtemperature)

Threading Model
---------------

* **Main State Machine Thread** (priority 0 - default): Coordinates overall process, blocks on semaphores waiting for operations to complete
* **Temperature Safety Thread** (priority 2): Continuously monitors thermistor, triggers e-stop if >120°C
* **Motor Control Thread** (priority 5): Executes SPI-based stepping commands from queue
* **Tachometer Monitoring Thread** (priority 2): Closed-loop fan speed control via PWM adjustment
* **LED Status Thread** (priority 5): Visual feedback (solid/slow blink/fast blink based on state)

Synchronization
---------------

* **Semaphores:** Motor completion signaling (state machine blocks until motors finish)
* **Mutexes:** SPI bus access protection, command queue protection
* **Atomic Variables:** E-stop flag (checked every state machine iteration)
* **Message Queues:** ISR-to-thread communication (through-beam sensor → servo control)

Key Features
============

* **Multi-threaded RTOS architecture** with proper priority-based scheduling
* **Safety-first design** with continuous temperature monitoring at highest priority
* **SPI-based motor control** (4 motors on shared bus)
* **Interrupt-driven sensors** for responsive wick detection and button timing
* **Closed-loop fan control** using tachometer feedback
* **Emergency stop** triggered automatically by overtemperature or manually via button
* **Wash cycle mode** for cleaning scent dispenser between batches

Building and Running
====================

Prerequisites
-------------

- Zephyr SDK installed
- STM32F446RE development board (Nucleo-F446RE)
- West build tool
- ST-Link programmer

Build Commands
--------------

Full system build:

.. code-block:: bash

    west build -b nucleo_f446re -p -- -DDTC_OVERLAY_FILE="${PWD}/nucleo_f446re.overlay"
    west flash


Project Structure
=================

::

    candle-maker/
    ├── src/
    │   ├── state_machine.c      # Main state machine and coordination
    │   ├── motors.c             # Stepper motor control (SPI-based)
    │   ├── heating.c            # Heating control + temp safety thread
    │   ├── cooling.c            # Fan control with tach feedback
    │   ├── servos.c             # Door lock and wick insertion servos
    │   └── sensors.c            # Button, limit switch, LED control
    │   ├── state_machine.h      # State definitions
    │   ├── motors.h             # Motor control interfaces
    │   ├── heating.h            # Heating + temperature monitoring
    │   ├── cooling.h            # Fan control interfaces
    │   ├── servos.h             # Servo control interfaces
    │   └── sensors.h            # Sensor interfaces
    ├── testing/
    │   ├── motor_test.c         # Individual motor testing
    │   ├── heating_test.c       # Temperature monitoring validation
    ├── nucleo_f446re.overlay    # Device tree configuration
    ├── prj.conf                 # Zephyr project configuration
    └── CMakeLists.txt           # Build configuration

Development Notes
=================

Design Decisions
----------------

**SPI Stepping**
    Motors are stepped by writing to DRV8434S registers over SPI rather than traditional GPIO STEP/DIR pins. This reduces GPIO usage (4 CS lines vs 8 GPIO pins) but increases CPU load and prevents simultaneous motor operation. **Not recommended for future designs** - use GPIO STEP/DIR instead.

**Blocking State Machine**
    Each state has some kind of blocking mechanism like motor control blocks on a semaphore until complete. This ensures strict sequencing and prevents race conditions.

**High-Priority Safety**
    Temperature monitoring runs at priority 2 to ensure it can preempt motor movements if dangerous conditions arise.

**ISR-Driven Wick Detection**
    Through-beam sensor uses interrupts and message queue for quicker response time.

Known Limitations
-----------------

.. warning::

    * **Hardware testing incomplete** - Firmware not validated on final PCB due to flashing issues
    * **Thermistor conversion not implemented** - Returns placeholder temperature (25°C)
    * **E-stop doesn't abort active motors** - Only prevents new operations from starting
    * **Fixed 30-second cooling** - Should be temperature-based instead

Future Improvements
===================

* Implement GPIO STEP/DIR motor control (recommended over SPI)
* Use hardware timers for step pulse generation (zero CPU overhead)
* Add true motor abort in e-stop (check flag every step)
* Temperature-based cooling completion with timeout safety
* Watchdog timer for automatic recovery from software failures
* Hardware e-stop button (direct power cutoff)
* Unit testing framework using Zephyr native_posix board

