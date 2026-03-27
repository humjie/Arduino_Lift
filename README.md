# Arduino_Lift

Assessment Description:

•	You are going to design a control system for a lift of a boutique hotel using the Arduino UNO MCUs. 
•	The hotel lobby is located on Level 1 and the guest rooms are located on Level 2 of the building. 
•	The basic components and circuitry have been provided to you. MCU-1 and MCU-2 are responsible for lobby operation and lift car control respectively.  The two MCUs are communicated via UART. The schematic diagrams are shown in Figure 1 & 2.
•	The Default Reset State of the lift is at Level 1, with the lift door closed.
•	The lift lobby operations (MCU-1) are as follows:
o	The Pushbutton Switch and the Limit Switch are the call buttons located in the Level 1 and Level 2 lift lobbies respectively.
o	When a call button is pressed, 
	Turn ON the corresponding LED: LED1 for Level 1, LED2 for Level 2.
	If the lift car is on a different level, move the lift to the level which the corresponding call button is pressed.
	If the lift car is on the correct level, 
•	Send a signal to MCU-2 to open the lift door. Normal lift door operations shall apply.
•	Blink the corresponding LED two times and then turn it OFF.  
	the system will only respond to a new call when a previous operation is completed.
o	The lift car is driven by a DC Motor (DCM) with Encoder. 
	Level 1  0°; Level 2  720° 
(Note: choose your own reference as the 0°) 
	Ascending: DCM rotates counterclockwise; Descending: clockwise
	Drive the DCM in 3 phases:
•	First 180°: 30% duty cycle
•	Next 360°: 90% duty cycle
•	Last 180°: 30% duty cycle
	The lift car shall only move when the lift door is closed.
•	The operations of the lift car (MCU-2) are as follows:
o	The control panel inside the lift car consists of a keypad and an OLED display
	The keypad is configured as follows:
•	Keys ‘1’ and ‘2’ are used to select the desirable level. 
o	Pressing n when the lift car is on Level n has no effect.
o	Send a signal to MCU-1 to move the lift car when a valid floor button is pressed.
•	Keys ‘#’ and ‘*’ are Door Open and Door Close buttons respectively.
•	Key ‘A’ is the alarm button. When it is pressed, the Piezo Buzzer beeps for ~1s.
•	The other keys have no function.
	The OLED display in the lift displays 
•	The current level (i.e. 1 or 2) when the lift is stationary.
•	“Going down” when the lift is descending.
•	“Going up” when the lift is ascending.
o	The lift door is controlled by a Stepper Motor. 
	Fully closed  0°; Fully opened  270° (Counterclockwise) 
(Note: choose your own reference as the 0°) 
	When the lift car is moving, the lift door shall not open.
	When the lift car is stationary and the lift door is closed, 
•	If Key ‘#’ OR the call button in the lobby on that floor is pressed, open the lift door, wait for 5s and then close the lift door.
	When the lift car is stationary and the lift door is opened,
•	If Key ‘#’ OR the call button in the lobby on that floor is pressed, the door shall remain open for an additional 5s;
•	If Key ‘*’ is pressed, the door shall close immediately.
	If the Opto-Switch is blocked when 
•	the lift door is open, the door shall remain opened, pressing Key ‘*’ has no effect on the door;
•	the lift door is closing, the operation shall be interrupted and open immediately.

•	You are to write a C++ language program to perform these tasks.
•	Use appropriate assumptions consistent with a real lift controller when you feel that there is ambiguity in any part of the tasks. I would accept variations if you can articulate your assumption(s) reasonably.
