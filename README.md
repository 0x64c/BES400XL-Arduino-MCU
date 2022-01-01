# BES400XL-Arduino-MCU
## Purpose:
To replace the microcontroller of a Breville BES400XL espresso machine with an Arduino; either to repair a broken machine or to make a working one customizable.

## Safety Warning:
This machine contains a live chassis. Always unplug the machine before programming the Arduino and never connect the circuit to a grounded appliance (such as a desktop PC) without proper isolation. Never touch the circuit or any of the machine's internal components while plugged in. Properly isolate and insulate the circuit to prevent damage to equipment and harm to persons.

This software is in active development and may contain bugs. Don't trust it until you have read and understood the code.

## Usage:
### LED's
The machine's state is indicated by the three LED's:

Power LED
|  State  |  Function |
|  :--:   |  :--:     |
|   OFF     |  Machine is off |
| Solid ON  |  Machine is on  |
| Blinking 2~4 times | Indication of the 2nd to 4th temperature settings |

Coffee LED
|  State  |  Function |
|  :--:   |  :--:     |
| Blinking | Heater temperature is below target brew temperature |
| Solid ON | Heater has reached the target brew temperature |

Steam LED
|  State  |  Function |
|  :--:   |  :--:     |
| Blinking | Steam mode is on and heater temperature is below target steam temperature |
| Solid ON | Steam mode is on and heater temperature has reached the target steam temperature |
### Buttons
The three buttons are used to control the machine's state:
* When the power is OFF, any button will wake the machine.
* When the power is ON, the power button will turn the machine off.
* When the power is ON, the machine is in brew mode by default. In this state, pressing the coffee button will start the brew cycle. Short pressing the steam button will cycle through temperature modes. Long pressing the steam button will engage steam mode.
* During the brew cycle, pressing either the steam or coffee buttons will stop the cycle.
* During steam mode, pressing either the steam or coffee buttons will return the machine to brew mode.

### Automatic Timers
* The shutdown timer will turn the machine OFF about 15 minutes after the last button input was detected.
* The brew timer will stop the brew cycle after about 40 seconds.
* The steam timer will return the machine to brew mode from steam mode after about 5 minutes. 

## Connection diagrams:

Remove the microcontroller from the power supply PCB of the espresso machine and wire the Arduino according to the following diagrams. An Arduino Nano is shown, but the pin references also apply to other models such as the Uno or Mega.

To power the arduino, connect the AC input to a 5V power supply to the live and neutral terminals and foam tape it to the inner wall of the machine. Connect the 5V output to the Arduino either through USB or the VIN pin terminal.

### Power Supply PCB:
```
                    U1
Ground        1           20
              2           19
              3           18
              4           17
              5           16
Pump output   6           15
Heater output 7           14
Steam LED     8           13
Coffee LED    9           12 Thermistor input
Power LED     10          11 Buttons input
```
### Arduino: (Nano)
```
                 D13           D12 Power LED
                 +3.3v         D11 Coffee LED
                 AREF          D10 Steam LED
Buttons input    A0             D9 Pump output
Thermistor input A1             D8 Heater output
                 A2             D7
                 A3             D6
                 A4             D4
                 A5             D4
                 A6             D3
                 A7             D2
                 +5V           GND
                 RESET       RESET
Ground           GND            D0
                 VIN            D1
```

## Current features:
* Heater is regulated (85/90/93/96°C for coffee, ~100°C for steam)
* Two main power states are handled (on and off)
* Front panel buttons control operation modes and are debounced
* Long button presses are detectable
* Selected brew temperature is indicated by the flashing of the power LED
* Power, heater and operation mode status is indicated by the LED's (solid, blinking, or off)
* Brewing cycle ends automatically after 40 seconds
* Steam cycle ends automatically after 5 minutes
* Timed automatic shut-off after 15 minutes
* Input sampling, temperature regulation, pump control and LED status are handled with a timer interrupt

## Todo:
* Connect a buzzer to the arduino and provide aural feedback?
* USB Serial control and feedback?
* Heater and brew time calibration
* Finetuning of the brew cycle duration based on output volume/weight
