# Warehouse Robot

A modular embedded warehouse robot prototype built around five ATmega16 microcontrollers. The system combines line-following navigation, ultrasonic obstacle detection, location identification, gripper control, and a central finite-state-machine controller.

The project was developed and simulated as an embedded-systems/robotics project using AVR C and Proteus.

## Overview

The robot is divided into five cooperating modules. Each module is responsible for a specific subsystem, while **Module 5** acts as the main sequencer and coordinates the overall task.

The intended workflow is:

1. Start the robot.
2. Follow a line toward the target shelf.
3. Monitor for obstacles while navigating.
4. Detect the target shelf location.
5. Open the gripper and pick up the item.
6. Close the gripper and confirm its position.
7. Navigate back to the home location.
8. Open the gripper to release the item.
9. Return to the idle state.

An emergency-stop state is triggered when an obstacle is detected during navigation.

## System Architecture

```text
                         +----------------------+
                         |      Module 5        |
                         |   Main Sequencer     |
                         |  ATmega16 + LCD      |
                         |   Finite State       |
                         |      Machine         |
                         +----------+-----------+
                                    |
                 +------------------+------------------+
                 |                  |                  |
                 v                  v                  v
        +----------------+  +----------------+  +----------------+
        |    Module 1    |  |    Module 2    |  |    Module 3    |
        | Line Following |  |    Obstacle    |  |   Location ID  |
        |  & Navigation  |  |   Detection    |  |                |
        +----------------+  +----------------+  +----------------+
                 |                  |                  |
                 v                  v                  v
             Motors             HC-SR04          4-bit Location
                                                   Identifier

                                    |
                                    v
                           +----------------+
                           |    Module 4    |
                           | Gripper Control|
                           | + Limit Switch |
                           +----------------+
```

The modules communicate through digital enable, command, and status signals. Module 5 controls the operating sequence and uses feedback from the other modules to advance through the task.

## Module Breakdown

### Module 1 — Line Following & Navigation

**ATmega16 — 8 MHz**

Module 1 controls the drive motors according to three line sensors:

* Left sensor
* Center sensor
* Right sensor

The controller implements simple sensor-based navigation:

* Center detected → drive straight
* Left detected → turn left
* Right detected → turn right
* No detected line → stop

The module also has an enable input controlled by Module 5 so that the motors remain stopped when navigation is not active.

### Module 2 — Obstacle Detection

**ATmega16 — 8 MHz**

Module 2 uses an **HC-SR04 ultrasonic sensor** to detect obstacles during navigation.

The implementation uses:

* Timer1
* Input Capture
* Interrupt Service Routine
* Rising/falling edge detection
* Echo pulse-duration measurement

The measured echo duration is converted to an approximate distance. An obstacle alert is generated when the detected distance is below the configured **20 cm threshold**.

The obstacle signal is monitored by Module 5 during navigation and can cause the robot to enter its emergency-stop state.

### Module 3 — Location Identification

**ATmega16 — 8 MHz**

Module 3 identifies the target shelf using a 4-bit location code.

The current implementation defines the target location as:

```text
1010
```

The module:

1. Reads the lower four bits of PORTB.
2. Compares them with the target location code.
3. Sends a location-found signal to Module 5 when a match occurs.
4. Activates a local feedback LED when the target is detected.

Scanning is enabled and disabled by Module 5.

### Module 4 — Gripper Control

**ATmega16 — 8 MHz**

Module 4 controls the robot's gripper motor and monitors two limit switches:

* Open limit switch
* Closed limit switch

The gripper receives an open/close command from Module 5 and reports its state through two dedicated status signals:

* `STATUS_IS_OPEN`
* `STATUS_IS_CLOSED`

When either limit switch is reached, the motor is stopped and the corresponding status signal is asserted.

### Module 5 — Main Sequencer

**ATmega16 — 8 MHz**

Module 5 is the central controller of the robot.

The control logic is implemented as a finite state machine with the following states:

```text
IDLE
    ↓
NAVIGATING_TO_SHELF
    ↓
AT_SHELF_STOPPED
    ↓
GRIPPER_OPENING
    ↓
PICKING_ITEM
    ↓
GRIPPER_CLOSING
    ↓
NAVIGATING_TO_HOME
    ↓
AT_HOME_STOPPED
    ↓
RELEASING_ITEM
    ↓
TASK_COMPLETE
    ↓
IDLE
```

An additional `EMERGENCY_STOP` state handles obstacle detection during navigation.

Module 5 also controls:

* Start input
* Module enable signals
* Gripper commands
* Gripper status feedback
* Busy/completion/error LEDs
* Waiting-for-gripper indicator
* 16×2 LCD status display

## Key Technical Details

### Finite-State Machine

The central controller separates the robot's operation into explicit states rather than implementing the entire task as one continuous control loop.

This makes the sequence of navigation, pickup, return, and release operations explicit and allows module feedback to determine state transitions.

### Inter-Module Handshaking

The modules use dedicated digital signals for coordination.

Examples include:

* Module 5 → Module 1: navigation enable
* Module 5 → Module 2: obstacle detection enable
* Module 5 → Module 3: location scanning enable
* Module 5 → Module 4: gripper command and enable
* Module 2 → Module 5: obstacle detected
* Module 3 → Module 5: target location found
* Module 4 → Module 5: gripper open/closed confirmation

### Ultrasonic Measurement

Module 2 uses the ATmega16 Timer1 Input Capture peripheral to measure the HC-SR04 echo pulse.

The capture interrupt alternates between:

1. Capturing the rising edge.
2. Capturing the falling edge.
3. Calculating the pulse duration.
4. Converting the duration to an approximate distance.

### Embedded Motor Control

Motor direction is controlled through digital outputs connected to motor-driver inputs. The individual modules directly control their assigned motors according to sensor inputs and commands from the main controller.

## Hardware

The project uses:

* ATmega16 microcontrollers
* Line-following sensors
* HC-SR04 ultrasonic sensor
* DC motors
* Motor drivers
* Gripper motor
* Open/closed limit switches
* LCD display
* Status LEDs
* Digital inter-module control signals

## Software

* AVR C
* AVR-GCC / Atmel Studio
* Proteus

Each ATmega16 module has its own source and project configuration.

## Project Structure

```text
warehouse-robot-project/
├── CODE/
│   ├── MODULE1/
│   │   └── MODULE1/
│   │       ├── main.c
│   │       └── MODULE1.cproj
│   ├── MODULE2/
│   │   └── MODULE2/
│   │       ├── main.c
│   │       └── MODULE2.cproj
│   ├── MODULE3/
│   │   └── MODULE3/
│   │       ├── main.c
│   │       └── MODULE3.cproj
│   ├── MODULE4/
│   │   └── MODULE4/
│   │       ├── main.c
│   │       └── MODULE4.cproj
│   ├── MODULE5/
│   │   └── MODULE5/
│   │       ├── main.c
│   │       └── MODULE5.cproj
│   └── UltraSonicTEP.HEX
│
├── WAREHOUSE ROBOT PROJECT.pdsprj
└── .gitignore
```

Generated compiler output and IDE cache files are excluded from the repository.

## Limitations

This project is an embedded robotics prototype rather than a production warehouse automation system.

The navigation logic is based on simple line-sensor decisions, and the target location is represented by a predefined digital location code. Obstacle handling currently transitions the controller into an emergency-stop state rather than implementing obstacle avoidance or path replanning.

The repository includes the AVR source code and Proteus project file used for the project. Detailed simulation configuration may depend on the original Proteus environment.

## Technologies

* **C**
* **AVR**
* **ATmega16**
* **Embedded Systems**
* **Timer1 Input Capture**
* **Interrupts / ISRs**
* **Finite-State Machines**
* **Ultrasonic Sensing**
* **Motor Control**
* **Proteus**
* **Atmel Studio / AVR-GCC**
