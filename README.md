# IoT Water Controller

A smart water level monitoring and automated pump control system powered by an **ESP32 Wi-Fi Microcontroller**, featuring an **HTTP Web Server**, **Real-Time Web Dashboard**, **REST JSON API**, 16x2 LCD display, HC-SR04 ultrasonic distance sensor, relay module, push button, and manual/auto selector switch.

---

## 🌟 Key Features

- **Wi-Fi & HTTP Web Server**: Connects to your local Wi-Fi network and serves a responsive Web Dashboard.
- **Real-Time Web Dashboard**: Displays live water level percentage gauge, pump status, control mode, distance reading, and target calibration.
- **REST JSON Status API**: Exposes JSON endpoints (`/api/status` & `/status`) for remote IoT integration, home automation (e.g. Home Assistant), or mobile apps.
- **Remote Pump Control**: Control the pump directly from your browser when in `MANUAL` mode.
- **Automatic & Manual Modes**: 
  - `AUTO Mode`: Automatically turns pump ON when water level falls below 30% and turns OFF when water level reaches 95%.
  - `MANUAL Mode`: Toggle pump on/off via physical push button or web dashboard.
- **EEPROM Storage**: Saves target calibration depth to EEPROM so settings persist across reboots.
- **LCD Display**: Local 16x2 LCD displays live percentage, Wi-Fi IP address, and operation status.

---

## 🛠️ Hardware Necessities Needed

1. **ESP32 Development Board** (e.g., ESP32 DevKit v1 / ESP-WROOM-32).
2. **HC-SR04 / HC-SR04P Ultrasonic Sensor** (Ultrasonic water depth measurement).
3. **16x2 LCD Display** + 10kΩ Potentiometer (for contrast adjustment).
4. **5V Relay Module** (Active-Low or Active-High, for pump switching).
5. **Push Button** (For manual toggle & target calibration).
6. **Slide Switch** (For selecting AUTO / MANUAL mode).
7. **Resistors**:
   - 1kΩ & 2kΩ resistors (used as a voltage divider to step down HC-SR04 Echo 5V signal to 3.3V for ESP32 safety).
   - 220Ω resistor (LCD backlight protection).
8. **Power Supply**: 5V DC supply (for relay & ultrasonic sensor) & Micro-USB / USB-C cable for ESP32.
9. **Breadboard & Jumper Wires**.

---

## 💻 Software Necessities Needed to Install

To build, flash, and run this project, install the following software tools:

### 1. Arduino IDE
- Download and install **Arduino IDE** (v2.x or v1.8.x): [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)

### 2. ESP32 Board Package (Espressif Systems)
1. Open Arduino IDE and go to **File > Preferences** (or `Ctrl + ,`).
2. Add the following URL to **Additional Boards Manager URLs**:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Go to **Tools > Board > Boards Manager...**.
4. Search for `esp32` (by Espressif Systems) and click **Install**.

### 3. USB-to-UART Serial Driver
Depending on your ESP32 board's USB chip, install the driver so your computer detects the board's COM port:
- **CP210x Driver**: [Silicon Labs CP210x Drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
- **CH340 / CH341 Driver**: [WCH CH340 Driver](http://www.wch-ic.com/downloads/CH341SER_EXE.html)

### 4. Required Arduino Libraries
- `WiFi.h` *(Built into ESP32 board core)*
- `WebServer.h` *(Built into ESP32 board core)*
- `EEPROM.h` *(Built into ESP32 board core)*
- `LiquidCrystal` *(Install via Arduino IDE: **Sketch > Include Library > Manage Libraries...**, search `LiquidCrystal`)*

---

## 📌 Pin Connections (ESP32)

| Component | Pin | ESP32 GPIO |
| :--- | :--- | :--- |
| **LCD Display** | RS | GPIO 2 |
| | Enable (E) | GPIO 3 *(Or GPIO 15/27 on physical hardware)* |
| | D4 | GPIO 4 |
| | D5 | GPIO 16 |
| | D6 | GPIO 17 |
| | D7 | GPIO 5 |
| **HC-SR04** | Trig | GPIO 18 |
| | Echo | GPIO 19 *(via 1kΩ/2kΩ voltage divider)* |
| **Relay Module** | Control (IN) | GPIO 23 |
| **Push Button** | Signal | GPIO 13 *(with internal `INPUT_PULLUP`)* |
| **Slide Switch** | Signal | GPIO 12 *(with internal `INPUT_PULLUP`)* |

---

## 🚀 Setup & Flashing Instructions

1. Open `sketch.ino` in **Arduino IDE**.
2. Update Wi-Fi credentials at lines 8–9:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
3. Select Board & Port:
   - Go to **Tools > Board > ESP32 Arduino > ESP32 Dev Module** (or your specific board model).
   - Go to **Tools > Port** and select your ESP32 COM port.
4. Upload the sketch (**Ctrl + U**).
5. Open Serial Monitor (**Ctrl + Shift + M**) at **115200 baud** to view Wi-Fi connection progress and IP address.
6. Open your web browser and navigate to `http://<ESP32_IP_ADDRESS>` to access the live dashboard!

---

## 🌐 Web Dashboard & REST API Endpoints

| Route | HTTP Method | Description |
| :--- | :--- | :--- |
| `/` | `GET` | HTML Web Dashboard with real-time progress gauge & controls |
| `/api/status` | `GET` | Returns JSON status payload (mode, pump, water_percent, distance, target, RSSI, uptime) |
| `/api/toggle` | `POST` / `GET` | Toggles pump state (only available in `MANUAL` mode) |
| `/api/set-target` | `POST` / `GET` | Saves current measured distance as reference target level |

### Sample JSON API Response (`/api/status`):
```json
{
  "mode": "AUTO",
  "pump": "OFF",
  "pump_boolean": false,
  "water_percent": 85,
  "distance_in": 15,
  "target_in": 100,
  "wifi_rssi": -62,
  "ip": "192.168.1.105",
  "uptime_sec": 1420
}
```

---

## ⚡ Simulation

You can simulate the circuit on [Wokwi](https://wokwi.com) using `diagram.json` and `wokwi-project.txt`.
