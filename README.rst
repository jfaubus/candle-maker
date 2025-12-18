# Candle Maker

An automated candle-making machine built with embedded systems and real-time operating system principles.

## Overview

This project automates the candle-making process from wax dispensing to cooling, using a state machine architecture running on Zephyr RTOS. The system coordinates multiple stepper motors, temperature control, and a display to guide users through each step of production.

## Hardware

- **Microcontroller:** STM32F446RE
- **Motor Drivers:** DRV8434S (SPI communication)
- **Motors:** 
  - Wax dispenser stepper motor
  - Scent dispenser stepper motor
  - Stirrer motor
- **Display:** LVGL-based screen for user interface
- **Temperature Control:** Heating element with monitoring

## Software Architecture

### State Machine
The system operates through distinct states, each handling a specific phase of candle production:

- `IDLE` - Waiting for user input
- `INIT_CHECK` - System initialization and safety checks
- `WAX_DISPENSE` - Dispensing wax into container
- `HEATING` - Heating wax to target temperature
- `SCENT_DISPENSE` - Adding scent to melted wax
- `STIRRING` - Mixing wax and scent
- `WICK_INSERT` - Positioning wick in container
- `COOLING` - Allowing candle to solidify
- `WASH_CYCLE` - Cleaning cycle between batches (if user holds button for more than five seconds)
- `ESTOP` - Emergency stop state
- `ENDSTATE` - Candle complete

### Threading Model
- **Main state machine thread:** Coordinates overall candle-making process
- **Display thread:** Handles UI updates and user input (non-blocking message queue communication)
- **Motor control threads:** Manage individual stepper motor operations
- **Temperature monitoring:** Tracks heating process

### Inter-Thread Communication
Threads communicate using Zephyr message queues (`k_msgq`) with non-blocking reads to ensure responsive operation.

## Key Features

- Real-time state visualization on display
- Emergency stop functionality
- Automated wash cycle for repeated production
- Thread-safe state transitions
- SPI-based motor control with daisy-chained drivers
- Temperature monitoring and control

## Building and Running

### Prerequisites
- Zephyr SDK installed
- STM32F446RE development board (Nucleo-F446RE)
- West build tool

### Build Commands
```bash
west build -b nucleo_f446re -p -- -DDTC_OVERLAY_FILE="${PWD}/nucleo_f446re.overlay"
west flash
```

## Project Structure
```
candle-maker/
├── src/
│   ├── stateMachine.c      # Main state machine logic and coordination
│   ├── display.c           # Display thread and UI handling
│   ├── motors.c            # Stepper motor control and SPI communication
│   ├── heating.c           # Heating control and temperature management
│   ├── cooling.c           # Cooling process control
│   ├── sensors.c           # Sensor monitoring and data acquisition
│   └── servos.c            # Servo motor control
│   ├── state_machine.h     # State definitions and shared interfaces
│   ├── display.h           # Display function declarations
│   ├── motors.h            # Motor control interfaces
│   ├── heating.h           # Heating control interfaces
│   ├── cooling.h           # Cooling control interfaces
│   ├── sensors.h           # Sensor interfaces
│   └── servos.h            # Servo control interfaces
├── nucleo_f446re.overlay   # Device tree overlay
└── prj.conf                # Zephyr project configuration
```

## Development Notes

- The display thread uses `K_NO_WAIT` for message queue reads to prevent blocking
- Each motor has independent control for precise coordination
- State transitions are managed centrally to ensure proper sequencing
- LVGL handles all display rendering with screen objects for each state

## Future Improvements

- Add state tracking
- better error recovery mechanisms
