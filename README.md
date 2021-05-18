# BES400XL-Arduino-MCU
Arduino Uno-based replacement of the microcontroller in the Breville BES400XL espresso machine 

Current features:
* Heat regulation (~90°C for coffee, ~100°C for steam)
* Power states (it turns on and off)
* Front panel button input (debounced) and LED indicators
* LEDs blink to show heater and mode status
* Pump automatically shuts off after ~40s
* Heater automatically cools down from steam mode after ~120s
* Preinfusion (pumps a bit of water into the basket on brew and startup)

Todo:
* Timed automatic shut-off
* Connect a buzzer to the arduino
* USB Serial control and feedback
* Heater and brew time calibration
