# Dual-ESP32 Medical Monitoring System

## Overview

A secure IoT medical monitoring system using two ESP32 microcontrollers with ESP-IDF framework for HACKFUSION 2026.

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         Cloud Server                         │
│            (Express.js + MongoDB + Web Dashboard)           │
└───────────────────────────▲─────────────────────────────────┘
                            │ HTTPS/TLS
                            │
                ┌───────────┴───────────┐
                │      Node A           │
                │  (Bio-Gateway)        │
                │                       │
                │  - MPU6050 (Accel)    │
                │  - HW827 (Heart)      │
                │  - DHT11 (Temp)       │
                │  - SOS Button         │
                │  - Fall Detection     │
                └───────────▲───────────┘
                            │ ESP-NOW (AES-128)
                            │
                ┌───────────┴───────────┐
                │      Node B           │
                │  (Sentry/Access)      │
                │                       │
                │  - RC522 RFID         │
                │  - IR Motion Sensor   │
                │  - Buzzer Alarm       │
                └───────────────────────┘
```

## Hardware Requirements

### Node A (Gateway)
- ESP32 DevKit (with WiFi)
- MPU6050 6-axis accelerometer/gyroscope (I2C)
- HW827 heart rate sensor (GPIO pulse detection)
- DHT11 temperature sensor (1-wire)
- Emergency button with pull-up resistor
- Status LED

### Node B (Sentry)
- ESP32 DevKit
- RC522 RFID reader (SPI)
- IR motion sensor (PIR or IR obstacle)
- Buzzer (PWM)
- Status LED

### Common
- 5V power supplies
- Breadboards and jumper wires
- Micro USB cables

## Software Requirements

- ESP-IDF v5.x ([Installation Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/))
- Node.js v16+ (for cloud server)
- MongoDB v5+ (local or Atlas)
- Git

## Quick Start

### 1. Setup ESP-IDF Environment

```bash
# Source ESP-IDF environment (do this in every terminal)
. $HOME/esp/esp-idf/export.sh
```

### 2. Configure Secrets

```bash
# Copy templates
cp shared/encryption_keys.h.template shared/encryption_keys.h
cp node-a-gateway/main/config.h.template node-a-gateway/main/config.h
cp node-b-sentry/main/config.h.template node-b-sentry/main/config.h
cp cloud-server/.env.template cloud-server/.env

# Edit each file with your credentials
# - WiFi SSID/password
# - Server URL
# - AES keys
# - Twilio credentials
```

### 3. Get ESP32 MAC Addresses

```bash
# Flash Node A
cd node-a-gateway
idf.py -p /dev/ttyUSB0 flash monitor

# Look for MAC address in logs: "Base MAC Address: XX:XX:XX:XX:XX:XX"
# Press Ctrl+] to exit monitor

# Repeat for Node B on different port
cd ../node-b-sentry
idf.py -p /dev/ttyUSB1 flash monitor
```

Update `shared/encryption_keys.h` with actual MAC addresses.

### 4. Build and Flash Nodes

```bash
# Sync shared protocol
cd shared
./sync.sh

# Build Node A
cd ../node-a-gateway
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor

# Build Node B (in new terminal)
cd ../node-b-sentry
idf.py build
idf.py -p /dev/ttyUSB1 flash monitor
```

### 5. Start Cloud Server

```bash
cd cloud-server
npm install
npm start
```

### 6. Access Dashboard

Open browser: `http://localhost:3000`

## Pin Configuration

### Node A GPIO Pins
| Component | GPIO | Description |
|-----------|------|-------------|
| I2C SDA | 21 | MPU6050 data |
| I2C SCL | 22 | MPU6050 clock |
| DHT11 | 4 | Temperature sensor (1-wire) |
| HW827 Pulse | 14 | Heart rate pulse input |
| SOS Button | 5 | Emergency button (active low) |
| Status LED | 2 | Built-in LED |

### Node B GPIO Pins
| Component | GPIO | Description |
|-----------|------|-------------|
| RFID SS | 5 | RC522 chip select |
| RFID RST | 27 | RC522 reset |
| SPI MOSI | 23 | RC522 data in |
| SPI MISO | 19 | RC522 data out |
| SPI SCK | 18 | RC522 clock |
| IR Sensor | 4 | Motion detector |
| Buzzer | 25 | PWM alarm |
| Status LED | 2 | Built-in LED |

## Features

### Node A (Bio-Gateway)
- ✅ Continuous heart rate monitoring (40-200 BPM)
- ✅ Body temperature tracking (35-42°C)
- ✅ Fall detection with 3-state algorithm
- ✅ Emergency SOS button
- ✅ Posture recognition (lying/sitting/standing)
- ✅ Receives encrypted data from Node B
- ✅ Sends aggregated data to cloud via HTTPS

### Node B (Sentry)
- ✅ RFID access control with whitelist
- ✅ IR motion detection with cooldown
- ✅ Buzzer alarm on intrusion/unauthorized access
- ✅ Sends encrypted events to Node A via ESP-NOW

### Cloud Server
- ✅ REST API for data ingestion
- ✅ MongoDB persistent storage
- ✅ Real-time WebSocket updates
- ✅ Twilio SMS alerts on emergencies
- ✅ Web dashboard with live charts

### Security
- ✅ AES-128-CBC encryption for ESP-NOW
- ✅ TLS/HTTPS for cloud communication
- ✅ API key authentication
- ✅ Certificate pinning (optional)

## MISRA-C Compliance

This project follows MISRA-C:2012 guidelines:
- Rule 8.4: All functions have prototypes
- Rule 14.4: All if-else chains have final else
- Rule 17.7: All return values checked
- Rule 10.3/10.4: Explicit type casts
- Rule 11.3: Pointer casts via uintptr_t
- Rule 21.1: No reserved identifiers
- Compiles with `-Wall -Wextra -Werror` (zero warnings)

## Testing

See [TESTING.md](TESTING.md) for detailed testing checklist.

### Manual Tests
```bash
# Monitor both nodes simultaneously
./tools/monitor-both.sh

# Flash individual nodes
./tools/flash-node-a.sh
./tools/flash-node-b.sh
```

### Integration Tests
1. Trigger fall detection: Drop Node A from 50cm height
2. Press SOS button: Should see alert on dashboard + SMS
3. Scan RFID: Authorized card should log access, unauthorized should buzz
4. Trigger IR sensor: Should see intrusion alert
5. Monitor vitals: Out-of-range values should trigger alerts

## Troubleshooting

### ESP32 won't flash
- Check USB cable (must be data cable, not charge-only)
- Hold BOOT button while connecting
- Verify port: `ls /dev/tty*`

### I2C sensor not detected
- Check wiring (SDA/SCL not swapped)
- Verify pull-up resistors (often built-in)
- Scan I2C bus: Use i2c-tools or ESP-IDF examples

### ESP-NOW not working
- Ensure both nodes use same WiFi channel
- Verify MAC addresses in `encryption_keys.h`
- Check distance (max ~100m line-of-sight)

### Cloud connection fails
- Verify WiFi credentials in `config.h`
- Check server URL and API key
- Test with `curl`: `curl -H "X-API-Key: your-key" http://server/api/dashboard/latest`

### Dashboard not updating
- Check MongoDB connection
- Verify WebSocket connection in browser console
- Ensure Node A can reach server (ping test)

## Development

### Build Options
```bash
# Clean build
idf.py fullclean && idf.py build

# Build with verbose output
idf.py -v build

# Parallel build (faster)
idf.py build -j8

# Configure project
idf.py menuconfig
```

### Debugging
```bash
# Enable verbose logs
idf.py menuconfig → Component config → Log output → Verbose

# Monitor with filters
idf.py monitor --print-filter="*:I NODE_A:V"

# View stack usage
# Add to task code: ESP_LOGI(TAG, "Stack: %d", uxTaskGetStackHighWaterMark(NULL));
```

## License

MIT License - HACKFUSION 2026 Project

## Contributors

- Shashvat Prabhu (Team Lead)

## Acknowledgments

- ESP-IDF Framework by Espressif Systems
- FreeRTOS for real-time multitasking
- Chart.js for dashboard visualizations
