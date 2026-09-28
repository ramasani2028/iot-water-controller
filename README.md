# IoT Water Controller

A water level monitoring and pump control project built with an Arduino Uno, HC-SR04 ultrasonic sensor, 16x2 LCD, relay module, push button, and slide switch.

> Note: This project currently uses local hardware control and does not include wireless network connectivity.

## Features

- Measures water level using an HC-SR04 ultrasonic sensor
- Displays water level percentage and pump status on a 16x2 LCD
- Supports manual and automatic pump control modes
- Saves the target water level setting in EEPROM for power cycling

## Hardware Components

- Arduino Uno
- HC-SR04 ultrasonic distance sensor
- 16x2 LCD display with LiquidCrystal interface
- 10K potentiometer for LCD contrast
- Relay module for pump control
- Push button for mode-specific actions
- Slide switch to select Manual / Auto mode
- 5V power supply and wiring

## Pin Connections

- LCD `RS` -> `D2`
- LCD `E` -> `D3`
- LCD `D4` -> `D4`
- LCD `D5` -> `D5`
- LCD `D6` -> `D6`
- LCD `D7` -> `D7`
- LCD `VSS` -> `GND`
- LCD `VDD` -> `5V`
- LCD `RW` -> `GND`
- LCD contrast `V0` -> Potentiometer output
- Potentiometer ends -> `5V` and `GND`

- HC-SR04 `VCC` -> `5V`
- HC-SR04 `GND` -> `GND`
- HC-SR04 `TRIG` -> `D8`
- HC-SR04 `ECHO` -> `D9`

- Push button -> `D10` (with `INPUT_PULLUP`; button connects to `GND` when pressed)
- Slide switch -> `D11` (with `INPUT_PULLUP`; switch connects to `GND` in one position)

- Relay control -> `D12`
- Relay `VCC` -> `5V`
- Relay `GND` -> `GND`

## Operation

- **Auto mode**: Slide switch enabled for auto, the controller automatically turns the pump on when water level falls below the low threshold and off when the tank is full.
- **Manual mode**: Slide switch set to manual, and button presses toggle the pump state directly.
- **Set target level**: In auto mode, press the button to store the current water level as the desired reference level. The target value is saved in EEPROM and retained across power cycles.

## Code Overview

- `sketch.ino` reads ultrasonic distance in inches and computes water level percentage using an EEPROM-stored reference level.
- `LiquidCrystal` manages the LCD display.
- `EEPROM` stores the desired water level reference so the system remembers it after restarting.

## IoT Upgrade

This project now includes a true IoT version using an ESP32:

- `sketch_esp32.ino` — ESP32 firmware with Wi-Fi, web dashboard, and JSON status API
- `sketch_legacy.ino` — original Arduino Uno version preserved for reference

### IoT features

- Connects to Wi-Fi and serves a web dashboard over HTTP
- Displays live water level, pump state, and mode
- Supports manual pump toggle from the browser in Manual mode
- Saves the target water level in EEPROM
- Provides a JSON status endpoint at `/status`

### Additional notes for ESP32 hardware

- Use a 3.3V-capable microcontroller such as ESP32 or ESP8266
- The HC-SR04 echo pin must be level-shifted before connecting to ESP32 input pins
- Relay modules should be powered from a stable 5V supply and share a common ground with the ESP32

## Simulation

- The original project can still be simulated using Wokwi with `wokwi-project.txt` or `diagram.json`.
- Required library: `LiquidCrystal`

## Files in this Project

- `sketch_ino` — Arduino Uno firmware source
- `sketch_esp32.ino` — ESP32 IoT firmware
- `sketch_legacy.ino` — preserved legacy Uno firmware copy
- `wokwi-project.txt` — Wokwi project export
- `diagram.json` — Wokwi circuit description
- `libraries.txt` — required library list

## Notes

- The upgraded project now supports true IoT operation with local web access and remote monitoring via the ESP32.
- Configure `ssid` and `password` inside `sketch_esp32.ino` before uploading.
