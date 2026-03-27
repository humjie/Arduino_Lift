# Arduino_Lift
- Description: A control system for a lift of a boutique hotel using the Arduino UNO MCUs. Course Project for MA2012 Introduction To Mechatronics Systems Design CA1.
- Teammates: Sean, Jack, Tze Nin, Rong Hao

## 🏨 Boutique Hotel Lift Control System (Arduino)
### 📖 Overview
This project implements a control system for a 2-level boutique hotel lift using two Arduino UNO microcontrollers communicating via UART.

- Level 1: Hotel Lobby

- Level 2: Guest Rooms

- Default System State: Lift at Level 1, doors closed.

The system is split between MCU-1 (handling external lobby calls and main lift car movement) and MCU-2 (handling internal lift car operations, displays, and door control).

### 🏗️ System Architecture & Hardware
1. MCU-1: Lobby Operations & Lift Motor Control
Hardware Components:

- Pushbutton Switch (Level 1 Call)

- Limit Switch (Level 2 Call)

- LED 1 (Level 1 Indicator)

- LED 2 (Level 2 Indicator)

- DC Motor with Encoder (Main Lift Drive)

2. MCU-2: Lift Car & Door Control
Hardware Components:

- 4x4 or 4x3 Keypad (Internal controls)

- OLED Display (Status monitor)

- Stepper Motor (Lift Door Drive)

- Piezo Buzzer (Alarm)

- Opto-Switch (Door safety sensor)

### ⚙️ Operational Logic
1. Lobby Operations (MCU-1)
MCU-1 handles external calls and the physical movement of the lift. Note: The system only responds to a new call when a previous operation is fully completed.

- Call Button Pressed:

  - Turns ON the corresponding LED (LED1 for L1, LED2 for L2).

  - If the lift is on a different level: Moves the lift to the called level.

  - If the lift is already on the correct level:

  - Sends a UART signal to MCU-2 to open the doors.

  - Blinks the corresponding LED twice, then turns it OFF.

- Lift Movement (DC Motor):

  - Constraint: The lift car shall only move when the lift door is closed.

  - Positioning: Level 1 = 0°, Level 2 = 720°.

  - Direction: Ascending = Counterclockwise (CCW), Descending = Clockwise (CW).

- Velocity Profile (3 Phases):

  - First 180°: 30% duty cycle

  - Next 360°: 90% duty cycle

  - Last 180°: 30% duty cycle

2. Lift Car Operations (MCU-2)
MCU-2 handles the internal UI, door mechanisms, and safety features.

- Internal Control Panel (Keypad):

  - 1 / 2: Select desirable level (ignored if already on that level). Sends UART signal to MCU-1 to initiate movement.

  - #: Door Open.

  - *: Door Close.

  - A: Alarm. Beeps Piezo Buzzer for ~1s.

- OLED Display States:

  - 1 or 2: When the lift is stationary.

  - Going up: When ascending.

  - Going down: When descending.

- Lift Door Control (Stepper Motor):

  - Positioning: Fully closed = 0°, Fully opened = 270° (CCW).

  - Constraint: Doors cannot open while the lift car is moving.

  - Stationary & Closed: If Key # or the Lobby Call button is pressed -> Open door, wait 5s, close door.

  - Stationary & Opened: * If Key # or Lobby Call button is pressed -> Add 5s to the open timer.

  - If Key * is pressed -> Close door immediately.

- Safety Interrupt (Opto-Switch):

  - If blocked while the door is open: Door remains open. Key * is ignored.

  - If blocked while the door is closing: Interrupt the closing operation and open the door immediately.
