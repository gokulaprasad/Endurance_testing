# Endurance Lab

Endurance Lab is a sophisticated project designed to automate the process of endurance testing using an ESP8266 microcontroller. It manages and logs multiple cycles of operations, sends data to a connected Raspberry Pi, and uses ESP-NOW for wireless communication.

## Table of Contents

- [Introduction](#introduction)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [License](#license)
- [Contributing](#contributing)

## Introduction

The Endurance Lab project is designed to facilitate endurance testing by automating the control and monitoring of cycles and logging relevant data. It uses a combination of an LCD, keypad, relay module, LEDs, buzzer, and ESP-NOW communication to provide a comprehensive testing solution.

## Features

- **Cycle Management**: Configures and controls the number of cycles and cycles per minute.
- **LCD and Keypad Input**: Uses an LCD and keypad for user input and feedback.
- **Data Logging**: Stores data in SPIFFS for resuming processes after interruptions.
- **ESP-NOW Communication**: Sends data wirelessly to another ESP8266 device.
- **Raspberry Pi Communication**: Sends data to a connected Raspberry Pi via serial communication.
- **Status Indicators**: Uses LEDs and a buzzer for visual and auditory feedback.

## Hardware Requirements

- ESP8266 Microcontroller
- LiquidCrystal I2C Display
- Keypad I2C
- Relay Module
- LEDs (Red, Yellow, Green)
- Buzzer
- SoftwareSerial (for Raspberry Pi communication)
- Wires and Connectors

## Software Requirements

- Arduino IDE
- ESP8266 Board Package
- Libraries: `LiquidCrystal_I2C`, `Keypad_I2C`, `Keypad`, `Wire`, `FS`, `ESP8266WiFi`, `espnow`, `SoftwareSerial`

## Installation

1. **Clone the Repository**:
    ```sh
    git clone https://github.com/yourusername/endurance-lab.git
    cd endurance-lab
    ```

2. **Install the Required Libraries**:
    Install the required libraries using the Arduino Library Manager or manually place them in the `libraries` folder of your Arduino environment.

3. **Upload the Code**:
    Open the `Endurance_Lab.ino` file in the Arduino IDE, select the correct board and port, and upload the code to your ESP8266.

## Usage

1. **Power On**: Power on the ESP8266.
2. **Enter Total Cycles**: Use the keypad to enter the total number of cycles.
3. **Enter Cycles Per Minute**: Use the keypad to enter the cycles per minute.
4. **Start Process**: The process will begin, and the current status will be displayed on the LCD.
5. **Monitor and Log**: The data will be logged and sent via ESP-NOW and to the connected Raspberry Pi.

## Configuration

### Pin Configuration

```cpp
#define I2C_Addr            0x20
#define RELAY_MODULE        D4
#define LED_Red             D5
#define LED_Yellow          D6
#define LED_Green           D7
#define BUZZER              D0
#define SPIFFS_TOTAL_CYCLE_COUNT      "/totalCycleCount.txt"
#define SPIFFS_REMAINING_CYCLE_COUNT  "/remainingCycleCount.txt"
#define SPIFFS_CPM                    "/cpm.txt"
#define SPIFFS_IS_FRESH_START         "/isFreshStart.txt"
```

### ESP-NOW Configuration

Update the MAC address of the receiver in the code:

```cpp
uint8_t receiverMACAddress[] = {0xc8, 0xc9, 0xa3, 0x06, 0x4d, 0xee};
```

## License

```plaintext
Copyright [2024] [Gokulaprasad]

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
