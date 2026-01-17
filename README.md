# Smart Irrigation System
(ongoing project)
A robust, multi-component smart irrigation system designed for Raspberry Pi. The system uses a state-machine based firmware to monitor soil moisture and control irrigation, coupled with a Qt-based GUI for remote monitoring and control over MQTT.

## System Architecture

The project is divided into two main components:

1.  **Pi Firmware (`pi/`)**: 
    - Written in C++20.
    - Implements a Finite State Machine (IDLE, MONITORING, WATERING, WAITING, ERROR, MANUAL).
    - Supports both real hardware and a simulated hardware layer for testing.
    - Uses MQTT for status reporting and command reception.
    - Logging via `spdlog`.

2.  **GUI Application (`src/`)**:
    - Developed with Qt 6.
    - Provides real-time charts of moisture levels.
    - Allows manual pump control and simulation scenario triggers.
    - Communicates with the Pi via MQTT.

## Features

- **Automated Irrigation**: Intelligent logic decides when to water based on filtered moisture readings and configurable thresholds.
- **Remote Monitoring**: Real-time status updates via MQTT.
- **Manual Override**: Direct control over the irrigation pump.
- **Simulation Mode**: Integrated simulator for testing logic without physical hardware.
- **Safety Mechanisms**: Automatic timeouts and emergency stop functionality.

## Requirements

### Pi Firmware
- CMake 3.14+
- C++20 Compiler
- Paho MQTT C library (`paho-mqtt3a`)
- `spdlog` (fetched automatically)
- GoogleTest (fetched automatically)

### GUI Application
- Qt 6 (Core, Gui, Widgets, Charts)
- Paho MQTT C library

## Setup and Installation

### Building the Pi Firmware
```bash
cd pi
mkdir build && cd build
cmake ..
make
```

### Building the GUI Application
```bash
mkdir build && cd build
cmake ..
make
```

## Running the System

1.  **Start the MQTT Broker**: Ensure Mosquitto or another MQTT broker is running (e.g., on the Pi).
2.  **Run the Pi Firmware**:
    ```bash
    ./pi/build/irrigation_system
    ```
3.  **Run the GUI**:
    ```bash
    ./build/smart_irrigation_system
    ```

## Testing

The Pi firmware includes unit and integration tests using GoogleTest.
```bash
cd pi/build
./irrigation_tests
```

## Screen shots
<img width="2560" height="1344" alt="image" src="https://github.com/user-attachments/assets/b2d38c76-2a23-43bc-922a-8245690817d2" />

<img width="2560" height="1344" alt="image" src="https://github.com/user-attachments/assets/74df9915-a6f7-48ca-a260-0330b7e95070" />

<img width="2012" height="1314" alt="image" src="https://github.com/user-attachments/assets/d7e35e92-4140-423a-9d46-10f229729d61" />



