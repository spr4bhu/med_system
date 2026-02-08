# Dual-ESP32 Medical Monitoring System

## Overview

A secure IoT medical monitoring and access control system using two ESP32 microcontrollers with ESP-IDF framework. Real-time vital monitoring with fall detection, posture recognition, and RFID-based access control.

### Architecture

```
┌──────────────────────────────────────────────────────────┐
│           HiveMQ Cloud MQTT Broker (TLS)                 │
│              mqtts://*.eu.hivemq.cloud:8883              │
└────────────────────────▲─────────────────────────────────┘
                         │ MQTT over TLS
                         │
         ┌───────────────┴──────────────┐
         │                              │
    ┌────▼──────────────┐      ┌───────▼─────────┐
    │    Node A         │      │    Node B       │
    │  (Vital Monitor)  │      │   (Sentry)      │
    │                   │      │                 │
    │ - MPU6050 (Fall)  │      │ - RC522 RFID    │
    │ - DHT11 (Temp)    │      │ - IR Motion     │
    │ - HW-827 (Heart)  │      │ - Buzzer Alarm  │
    │ - Button (SOS)    │      │                 │
    └──────────┬────────┘      └────────┬────────┘
               │ ESP-NOW P2P           │
               └───────────────────────┘
                  (Emergency alerts)
```

## Hardware Requirements

### Node A: Vital Monitor (Medical Monitoring)
- **Microcontroller**: ESP32 DevKit V1
- **Sensors**:
  - MPU6050 (I2C: GPIO 21/22) - 6-axis IMU for fall detection + posture
  - DHT11 (GPIO 5) - Temperature & humidity (1-wire protocol)
  - HW-827 (GPIO 36, ADC) - Heart rate sensor
  - Emergency Button (GPIO 4, active LOW) - Manual SOS trigger

### Node B: Sentry (Access Control & Security)
- **Microcontroller**: ESP32 DevKit V1
- **Sensors**:
  - RC522 RFID Reader (SPI: GPIO 23/25/19/22) - Access control
  - IR Motion Sensor (GPIO 13) - Intrusion detection
  - Buzzer (GPIO 5, NPN driver) - Alarm output

### Common Requirements
- 3.3V power via AMS1117 regulator
- USB-C or Li-ion battery (3.7-4.2V)
- Breadboards and jumper wires
- Git and ESP-IDF 5.1+

## Software Requirements

- **ESP-IDF v5.1+** - [Installation Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- **Git** - For version control
- **HiveMQ Cloud Account** - Free tier MQTT broker

## Quick Start

### 1. Setup ESP-IDF

```bash
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32
source export.sh
```

### 2. Clone Repository

```bash
cd ~
git clone https://github.com/vruga/esp_medmonitor.git
cd esp_medmonitor
git checkout shashvatt
```

### 3. Configure Credentials

```bash
cp node-a-gateway/main/config.h.template node-a-gateway/main/config.h
cp node-b-sentry/main/config.h.template node-b-sentry/main/config.h

# Edit with your WiFi and HiveMQ credentials
nano node-a-gateway/main/config.h
nano node-b-sentry/main/config.h
```

### 4. Build and Flash Node A

```bash
cd node-a-gateway
idf.py build
idf.py -p /dev/cu.usbserial-XXXXX flash monitor
```

### 5. Build and Flash Node B

```bash
cd ../node-b-sentry
idf.py build
idf.py -p /dev/cu.usbserial-YYYYY flash monitor
```

## Pin Configuration

### Node A GPIO Mapping
| Component | GPIO | Protocol | Purpose |
|-----------|------|----------|---------|
| MPU6050 SDA | 21 | I2C | Accelerometer/Gyroscope data |
| MPU6050 SCL | 22 | I2C | Accelerometer/Gyroscope clock |
| DHT11 Data | 5 | 1-Wire | Temperature & humidity |
| HW-827 ADC | 36 | ADC1_CH0 | Heart rate sensor |
| Emergency Button | 4 | GPIO Input | SOS button (active LOW) |
| UART TX | 1 | UART | Serial output (115200 baud) |
| UART RX | 3 | UART | Serial input |

### Node B GPIO Mapping
| Component | GPIO | Protocol | Purpose |
|-----------|------|----------|---------|
| RC522 MOSI | 23 | SPI | RFID data in |
| RC522 MISO | 25 | SPI | RFID data out |
| RC522 SCLK | 19 | SPI | RFID clock |
| RC522 CS | 22 | SPI | RFID chip select |
| IR Sensor | 13 | GPIO Input | Motion detection |
| Buzzer | 5 | GPIO Output | Alarm (NPN driver) |
| UART TX | 1 | UART | Serial output |
| UART RX | 3 | UART | Serial input |

## Features

### Node A: Vital Monitoring
- ✅ **Fall Detection**: Jerk-based algorithm (200 g/s threshold) with 2s cooldown
- ✅ **Posture Recognition**: Complementary filter (99% gyro + 1% accel)
  - STANDING: pitch ≤ 15°
  - SITTING: pitch ≥ 20°
  - LYING: pitch & roll ≤ 10°
- ✅ **Heart Rate Monitoring**: Peak detection on HW-827 sensor
- ✅ **Temperature Tracking**: DHT11 with retry logic (WiFi interference tolerance)
- ✅ **Emergency Button**: Manual SOS trigger
- ✅ **MQTT Publishing**: Real-time sensor data to HiveMQ Cloud
- ✅ **ESP-NOW**: Emergency messages to Node B (dynamic channel)

### Node B: Access Control & Security
- ✅ **RFID Access Control**: RC522 reader with full SPI protocol
  - Request → Anticollision → UID database lookup
  - Authorized/Unauthorized card detection
- ✅ **IR Motion Detection**: PIR sensor with state machine
- ✅ **Security State Machine**: IDLE → IR_DETECTED → AUTHORIZED/INTRUSION
- ✅ **Buzzer Alarm**: GPIO-driven NPN transistor
- ✅ **MQTT Publishing**: Security events to HiveMQ Cloud
- ✅ **ESP-NOW Receiver**: Listens for emergencies from Node A

### Network & Communication
- ✅ **WiFi**: STA mode with auto-reconnect
- ✅ **MQTT over TLS**: HiveMQ Cloud with certificate verification
- ✅ **SNTP**: Time synchronization via pool.ntp.org
- ✅ **ESP-NOW**: Dynamic channel detection (follows WiFi AP channel)
- ✅ **DHT11 Retry**: Up to 3 retries on checksum errors

## MISRA-C Compliance

Code follows MISRA-C:2012 guidelines:
- ✅ Rule 8.4: Function prototypes in headers
- ✅ Rule 14.4: All if-else chains have final else
- ✅ Rule 17.7: All return values checked
- ✅ Rule 10.3/10.4: Explicit type casts
- ✅ Rule 2.7: Unused parameters marked `(void)param`
- ✅ Rule 9.1: Variables initialized before use
- ✅ Compiles with `-Wall -Wextra -Werror`

## Testing

### Quick Test Setup
```bash
# Terminal 1: Node A
cd node-a-gateway && idf.py -p /dev/cu.usbserial-A monitor

# Terminal 2: Node B
cd node-b-sentry && idf.py -p /dev/cu.usbserial-B monitor
```

### Test Cases
1. **Fall Detection**: Shake/drop Node A → Watch for "FALL DETECTED"
2. **Posture**: Rotate Node A → Verify posture changes (STANDING/SITTING/LYING)
3. **Heart Rate**: Simulate sensor pulses → Peak count increases
4. **RFID Authorized**: Scan authorized card → Buzzer silent
5. **RFID Unauthorized**: Scan unknown card → Buzzer sounds
6. **Emergency**: Press button on Node A → Node B buzzer sounds
7. **MQTT**: Check HiveMQ console for all messages

## Troubleshooting

### ESP32 won't flash
```bash
ls /dev/cu.usbserial-*
# Hold BOOT button while connecting
```

### DHT11 timeouts
- External 2.2kΩ pull-up required on GPIO 5
- Keep wire short (<10cm)
- Check 3.3V power

### ESP-NOW not working
- Both nodes must be on same WiFi channel
- Verify MAC addresses match actual hardware
- Both must be powered on WiFi

### MQTT connection fails
- Check WiFi SSID/password in config.h
- Verify HiveMQ credentials
- Ensure TLS certificate is correct

### RFID not detected
- Check SPI wiring (MOSI/MISO/SCLK)
- Verify RC522 at 3.3V (not 5V)
- Try increasing SPI clock if needed

## Build Options

```bash
idf.py fullclean && idf.py build
idf.py -v build                    # Verbose output
idf.py build -j8                   # Parallel build
idf.py menuconfig                  # Configuration
idf.py monitor --timestamp         # Monitor with timestamps
```

## Project Structure

```
esp_medmonitor/
├── node-a-gateway/          # Vital Monitor firmware
│   ├── main/
│   │   ├── config.h.template
│   │   ├── sensors/         # MPU6050, DHT11, HW-827
│   │   ├── tasks/           # Fall detection, sensor, MQTT
│   │   └── utils/
│   └── CMakeLists.txt
├── node-b-sentry/           # Access Control firmware
│   ├── main/
│   │   ├── config.h.template
│   │   ├── modules/         # RFID, IR, Buzzer
│   │   ├── tasks/           # Security, MQTT
│   │   └── utils/
│   └── CMakeLists.txt
├── Hackthon_PCB/            # KiCAD schematics & layouts
│   ├── Hackthon_PCB.kicad_sch
│   ├── Hackthon_PCB.kicad_pcb
│   ├── gerbers/
│   └── kicad_parts/
└── README.md
```
