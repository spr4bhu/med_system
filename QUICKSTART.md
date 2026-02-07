# Quick Start Guide

## Prerequisites

1. **ESP-IDF v5.x** installed
   - Follow: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
   - Verify: `idf.py --version`

2. **Two ESP32 DevKit boards** with USB cables

3. **Hardware components**:
   - Node A: MPU6050, HW827, DHT11, button
   - Node B: RC522 RFID, IR sensor, buzzer

## Step-by-Step Setup (15 minutes)

### 1. Configure Secrets (5 min)

```bash
# Navigate to project
cd esp_medmonitor

# Copy configuration templates
cp shared/encryption_keys.h.template shared/encryption_keys.h
cp node-a-gateway/main/config.h.template node-a-gateway/main/config.h
cp node-b-sentry/main/config.h.template node-b-sentry/main/config.h

# Edit Node A config
nano node-a-gateway/main/config.h
# Update:
#   - WIFI_SSID "YourNetworkName"
#   - WIFI_PASSWORD "YourPassword"
#   - CLOUD_SERVER_URL "http://yourserver.com/api/data"
#   - API_KEY "your-api-key"

# Edit Node B config (if needed)
nano node-b-sentry/main/config.h
# Update:
#   - AUTHORIZED_UIDS[] with your RFID card UIDs

# Edit encryption keys
nano shared/encryption_keys.h
# Change AES_KEY to a random 16-byte key
# Leave MAC addresses as placeholders for now
```

### 2. Get MAC Addresses (2 min)

```bash
# Source ESP-IDF
. $HOME/esp/esp-idf/export.sh

# Flash Node A minimal firmware
cd node-a-gateway
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
# Look for: "Base MAC Address: XX:XX:XX:XX:XX:XX"
# Press Ctrl+] to exit

# Flash Node B
cd ../node-b-sentry
idf.py -p /dev/ttyUSB1 flash monitor
# Look for MAC address
# Press Ctrl+] to exit

# Update shared/encryption_keys.h with actual MAC addresses
cd ..
nano shared/encryption_keys.h
```

### 3. Sync Shared Protocol (1 min)

```bash
cd shared
chmod +x sync.sh
./sync.sh
```

### 4. Build Both Nodes (5 min)

```bash
# Build Node A
cd ../node-a-gateway
idf.py build

# If successful, build Node B
cd ../node-b-sentry
idf.py build
```

### 5. Flash and Test (2 min)

```bash
# Flash Node A
cd ../node-a-gateway
idf.py -p /dev/ttyUSB0 flash monitor

# In another terminal, flash Node B
cd node-b-sentry
idf.py -p /dev/ttyUSB1 flash monitor
```

## Troubleshooting

### Build Errors

**Error**: `config.h: No such file or directory`
- **Fix**: Copy `config.h.template` to `config.h`

**Error**: `encryption_keys.h: No such file or directory`
- **Fix**: Run `cd shared && ./sync.sh`

**Error**: Compiler warnings about unused variables
- **Fix**: Check that you copied the latest code (all warnings should be handled)

### Flash Errors

**Error**: `Failed to connect to ESP32`
- **Fix**: Hold BOOT button while connecting USB
- **Fix**: Check USB cable (must support data, not just charging)
- **Fix**: Verify port: `ls /dev/tty*`

**Error**: Permission denied on `/dev/ttyUSB0`
- **Fix**: `sudo usermod -a -G dialout $USER` (then logout/login)
- **Fix**: Or use `sudo` before idf.py commands

### Runtime Errors

**Error**: Node A can't connect to WiFi
- **Fix**: Double-check SSID/password in `config.h`
- **Fix**: Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)

**Error**: ESP-NOW not working
- **Fix**: Verify MAC addresses in `encryption_keys.h`
- **Fix**: Ensure both nodes on same WiFi channel

**Error**: I2C sensor not detected
- **Fix**: Check wiring (SDA to GPIO 21, SCL to GPIO 22)
- **Fix**: Verify sensor address (0x68 for MPU6050)
- **Fix**: HW827 uses GPIO 14 for pulse detection, not I2C

## Minimal Test (No Hardware)

To verify firmware compiles and runs (without sensors):

```bash
cd node-a-gateway
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor

# You should see:
# - NVS initialized
# - WiFi started
# - Sensor init (may fail without hardware - OK)
# - All tasks created
# - "All systems initialized - Node A running"
```

Even without sensors connected, the firmware will start and run (sensors will report default values).

## Next Steps

1. **Hardware Assembly**: Connect all sensors per README.md pin table
2. **Cloud Server**: Implement backend using Phase 5-6 from plan
3. **Testing**: Follow TESTING.md checklist
4. **Demo**: Prepare fall detection, RFID, intrusion demos

## Quick Commands Reference

```bash
# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash

# Monitor
idf.py -p /dev/ttyUSB0 monitor

# Flash + Monitor
idf.py -p /dev/ttyUSB0 flash monitor

# Clean build
idf.py fullclean && idf.py build

# Configure (menuconfig)
idf.py menuconfig

# Erase flash
idf.py -p /dev/ttyUSB0 erase-flash
```

## Help & Support

- **ESP-IDF Docs**: https://docs.espressif.com/projects/esp-idf/
- **FreeRTOS API**: https://www.freertos.org/a00106.html
- **Troubleshooting**: See TESTING.md

## Success Checklist

- [ ] ESP-IDF v5.x installed and working
- [ ] Both nodes compile with zero warnings
- [ ] MAC addresses extracted and updated
- [ ] Node A connects to WiFi
- [ ] Node A prints "All systems initialized"
- [ ] Node B prints "All systems initialized"
- [ ] Sensors read (even if no hardware, should not crash)

If all checked, you're ready for hardware integration! 🎉
