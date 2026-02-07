# Testing Checklist

## Build Verification

### Compilation Tests
- [ ] Node A compiles without warnings (`idf.py build 2>&1 | grep warning` returns empty)
- [ ] Node B compiles without warnings
- [ ] Zero errors in static analysis
- [ ] All tasks fit within stack size (check high water marks)

## MISRA-C Compliance Verification

### Rule 8.4 - Function Prototypes
- [ ] All sensor functions declared in `.h` files
- [ ] All task functions have prototypes
- [ ] No implicit function declarations

### Rule 14.4 - Final Else Clause
- [ ] Fall detection state machine has default case
- [ ] Vitals monitoring if-else chains complete
- [ ] Access control logic has final else

### Rule 17.7 - Return Value Checking
- [ ] All `esp_err_t` return values checked
- [ ] All `BaseType_t` (FreeRTOS) checked for pdPASS
- [ ] Queue operations check return values
- [ ] I2C/SPI operations check errors

### Rule 10.3/10.4 - Type Conversions
- [ ] No implicit narrowing conversions
- [ ] All casts explicit with type annotation
- [ ] Compile with `-Wconversion` passes

### Rule 11.3 - Pointer Casts
- [ ] All pointer-to-integer via `uintptr_t`
- [ ] No direct `(uint32_t)ptr` casts

### Rule 21.1 - Reserved Identifiers
- [ ] No leading underscores in variable names
- [ ] No reserved C library name conflicts

## Hardware Tests - Node A

### MPU6050 Accelerometer
- [ ] I2C initialization successful (address 0x68 detected)
- [ ] Accelerometer reads valid data
- [ ] X-axis: -2g to +2g range
- [ ] Y-axis: -2g to +2g range
- [ ] Z-axis: Shows ~1g when flat (gravity)
- [ ] Data updates at 100Hz

### HW827 Heart Rate Sensor
- [ ] GPIO initialization successful
- [ ] Pulse detection ISR working
- [ ] BPM reading between 40-200
- [ ] Stable reading after 10 seconds
- [ ] No false pulse detections

### DHT11 Temperature Sensor
- [ ] Reads ambient temperature (±2°C accuracy)
- [ ] Temperature in range 15-35°C (room temp)
- [ ] No CRC errors
- [ ] 1-wire protocol timing correct

### Emergency Button
- [ ] Button press detected
- [ ] Debouncing works (no multiple triggers)
- [ ] Sets SOS_PRESSED_BIT in event group
- [ ] ISR executes without crash

### Fall Detection Algorithm
- [ ] State machine: IDLE → FREE_FALL → IMPACT → CONFIRMED
- [ ] Drop test: Detecting fall from 50cm height
- [ ] Free-fall phase: magnitude < 0.5g for >50ms
- [ ] Impact phase: magnitude > 2.5g within 500ms
- [ ] No false positives from normal movement
- [ ] 5-second cooldown works

### WiFi Connectivity
- [ ] Connects to WiFi AP
- [ ] DHCP obtains IP address
- [ ] Ping server successful
- [ ] Reconnects after disconnect

## Hardware Tests - Node B

### RC522 RFID Reader
- [ ] SPI initialization successful
- [ ] Detects card presence
- [ ] Reads UID correctly
- [ ] Authorized card grants access
- [ ] Unauthorized card denied
- [ ] Buzzer sounds on unauthorized (1 second)

### IR Motion Sensor
- [ ] Detects motion/presence
- [ ] GPIO interrupt fires
- [ ] Cooldown timer works (5 seconds)
- [ ] No false triggers from vibration

### Buzzer Alarm
- [ ] PWM initialization successful
- [ ] Sounds at 2000 Hz
- [ ] Intrusion alarm: 3 seconds
- [ ] Unauthorized access: 1 second
- [ ] Auto-off after duration

### ESP-NOW Transmitter
- [ ] ESP-NOW initializes
- [ ] Sends encrypted packets to Node A
- [ ] Packet format correct (`espnow_packet_t`)
- [ ] CRC16 calculated correctly
- [ ] Retry mechanism works (3 attempts)

## Communication Tests

### ESP-NOW (Node B → Node A)
- [ ] Node A receives packets from Node B
- [ ] AES-128-CBC decryption successful
- [ ] CRC16 validation passes
- [ ] Latency < 50ms
- [ ] No packet loss in 100 consecutive transmissions
- [ ] Works at 10m distance
- [ ] Works at 50m distance (optional)

### HTTPS (Node A → Cloud)
- [ ] TLS handshake successful
- [ ] Certificate validation passes
- [ ] JSON payload sent correctly
- [ ] Server responds with 200 OK
- [ ] API key authentication works
- [ ] Latency < 2 seconds
- [ ] Handles network interruption gracefully

## FreeRTOS Task Tests

### Node A Tasks
- [ ] All 5 tasks created successfully
- [ ] Sensor task runs at 100ms interval
- [ ] Fall detection task priority 6 (highest)
- [ ] Vitals monitoring task runs at 1s interval
- [ ] Gateway RX task processes ESP-NOW callbacks
- [ ] Cloud TX task sends data every 2 seconds
- [ ] No stack overflows (check high water marks)
- [ ] No task starvation

### Node B Tasks
- [ ] All 3 tasks created successfully
- [ ] Access control task scans RFID every 500ms
- [ ] Intrusion detection task priority 6 (highest)
- [ ] ESP-NOW TX task sends events reliably
- [ ] No stack overflows

### Synchronization
- [ ] Queue operations don't block indefinitely
- [ ] Semaphore (i2c_mutex) prevents race conditions
- [ ] Event groups set/clear correctly
- [ ] No deadlocks after 30 minutes runtime

## Cloud Server Tests

### Backend API
- [ ] Server starts on port 3000
- [ ] MongoDB connection successful
- [ ] POST `/api/sensor-data` accepts data
- [ ] POST `/api/events` accepts events
- [ ] GET `/api/dashboard/latest` returns data
- [ ] GET `/api/dashboard/history` returns historical data
- [ ] Authentication middleware blocks unauthorized requests
- [ ] Decryption middleware works correctly

### Database
- [ ] `SensorData` documents saved correctly
- [ ] `Event` documents saved correctly
- [ ] `AccessLog` documents saved correctly
- [ ] Timestamps stored in ISO format
- [ ] Queries return correct results

### WebSocket
- [ ] Socket.io connection established
- [ ] `sensor_data` events broadcast in real-time
- [ ] `event` events broadcast in real-time
- [ ] Multiple clients can connect simultaneously
- [ ] Reconnection after disconnect works

### Twilio SMS
- [ ] SMS sent on fall detection
- [ ] SMS sent on intrusion
- [ ] SMS sent on vitals abnormal
- [ ] SMS contains correct message
- [ ] SMS arrives within 30 seconds

## Web Dashboard Tests

### UI Rendering
- [ ] Page loads without errors
- [ ] Header displays "Medical Monitor Dashboard"
- [ ] Connection status shows "Connected"
- [ ] All charts render correctly

### Real-time Updates
- [ ] Heart rate chart updates within 2 seconds
- [ ] Temperature chart updates within 2 seconds
- [ ] Posture display updates correctly
- [ ] Recent events list populates
- [ ] Access control log table fills

### Charts
- [ ] Heart rate chart: Y-axis 40-200 BPM
- [ ] Temperature chart: Y-axis 34-42°C
- [ ] Chart auto-scrolls after 50 data points
- [ ] Chart colors match severity (red for critical)

### Alerts
- [ ] Fall detection shows red banner
- [ ] Intrusion shows orange banner
- [ ] Alert banners auto-dismiss after 10 seconds
- [ ] Alert banner animates (pulse)

### Historical Data
- [ ] Page load fetches last 1 hour data
- [ ] Historical data populates charts correctly
- [ ] Access logs show latest 100 entries
- [ ] Events show latest 50 entries

## Integration Tests

### End-to-End Flow: Fall Detection
1. [ ] Drop Node A from 50cm height
2. [ ] Fall detected within 500ms
3. [ ] ESP-NOW packet sent (if Node B running)
4. [ ] HTTPS POST to cloud successful
5. [ ] Dashboard shows "FALL DETECTED" in red
6. [ ] SMS alert received on phone
7. [ ] Event logged to MongoDB
8. [ ] Event appears in "Recent Events" list

### End-to-End Flow: Intrusion
1. [ ] Trigger IR sensor on Node B
2. [ ] Buzzer sounds for 3 seconds
3. [ ] ESP-NOW packet sent to Node A
4. [ ] Node A forwards to cloud via HTTPS
5. [ ] Dashboard shows intrusion alert
6. [ ] SMS alert received
7. [ ] Event logged to MongoDB

### End-to-End Flow: RFID Access
1. [ ] Scan authorized RFID card
2. [ ] Access granted (no buzzer)
3. [ ] ESP-NOW packet sent to Node A
4. [ ] Node A forwards to cloud
5. [ ] Dashboard access log shows "Granted" in green
6. [ ] AccessLog document in MongoDB

### End-to-End Flow: Unauthorized RFID
1. [ ] Scan unauthorized RFID card
2. [ ] Buzzer sounds for 1 second
3. [ ] ESP-NOW packet sent
4. [ ] Dashboard shows "Denied" in red
5. [ ] Event logged

### End-to-End Flow: SOS Button
1. [ ] Press SOS button on Node A
2. [ ] Emergency event triggered
3. [ ] HTTPS POST to cloud
4. [ ] Dashboard shows critical alert
5. [ ] SMS alert sent

## Performance Tests

### Stability
- [ ] System runs for 30 minutes without crash
- [ ] System runs for 1 hour without crash (optional)
- [ ] Free heap stable (no memory leaks)
- [ ] Stack high water marks > 512 bytes

### Latency
- [ ] Sensor read to queue: < 10ms
- [ ] Fall detection: < 500ms
- [ ] ESP-NOW transmission: < 50ms
- [ ] HTTPS POST: < 2 seconds
- [ ] WebSocket update: < 2 seconds
- [ ] End-to-end (sensor to dashboard): < 5 seconds

### Throughput
- [ ] Node A handles 10 sensor readings/second
- [ ] Node B handles 2 RFID scans/second
- [ ] Cloud server handles 10 requests/second
- [ ] Dashboard updates 1 time/second

### Power Consumption (Optional)
- [ ] Measure idle current
- [ ] Measure active current
- [ ] Estimate battery life with 5000mAh

## Security Tests

### Encryption
- [ ] AES-128-CBC encrypts data correctly
- [ ] Decryption recovers original data
- [ ] IV changes each transmission (no reuse)
- [ ] Corrupted ciphertext fails CRC check

### Authentication
- [ ] Wrong API key returns 401 Unauthorized
- [ ] Missing API key returns 401
- [ ] Correct API key returns 200 OK

### TLS/HTTPS
- [ ] Certificate validation enabled
- [ ] Self-signed certificate rejected (if pinning enabled)
- [ ] Man-in-the-middle attack prevented

## Failure Mode Tests

### Node A Failures
- [ ] MPU6050 disconnected: System continues (no crash)
- [ ] HW827 disconnected: System continues
- [ ] DHT11 disconnected: System continues
- [ ] WiFi disconnected: Automatic reconnection
- [ ] Server unreachable: Retry mechanism works

### Node B Failures
- [ ] RC522 disconnected: System continues
- [ ] IR sensor disconnected: System continues
- [ ] Buzzer disconnected: System continues (silent)
- [ ] Node A unreachable: ESP-NOW retry works

### Recovery
- [ ] Power cycle: System restarts and reconnects
- [ ] Watchdog timer resets frozen system
- [ ] Error logs visible in console

## Code Quality Checks

### Static Analysis
```bash
# Run in each node directory
grep "warning:" build_output.txt  # Should be empty
grep "error:" build_output.txt    # Should be empty
```

### MISRA Violations
```bash
# Check for leading underscores (Rule 21.1)
grep -r "^_" main/  # Should be empty

# Check for implicit conversions
# Compile with -Wconversion -Wsign-conversion
```

### Memory Safety
- [ ] No buffer overflows
- [ ] No use-after-free
- [ ] No null pointer dereferences
- [ ] All arrays bounds-checked

## Documentation Tests

- [ ] README.md accurate and complete
- [ ] TESTING.md (this file) complete
- [ ] Pin configuration documented
- [ ] API endpoints documented
- [ ] Code comments clear and helpful

## Final Demo Checklist

### Before Demo
- [ ] All hardware connected and powered
- [ ] Both nodes flashed with latest firmware
- [ ] Cloud server running
- [ ] Dashboard accessible on laptop
- [ ] Phone ready to receive SMS
- [ ] Backup power source ready

### Demo Flow
1. [ ] Show dashboard with live vitals
2. [ ] Demonstrate fall detection (drop test)
3. [ ] Show SMS alert on phone
4. [ ] Demonstrate SOS button
5. [ ] Show RFID access control (authorized + unauthorized)
6. [ ] Demonstrate intrusion detection
7. [ ] Show historical charts
8. [ ] Explain encryption and security

### Backup Plans
- [ ] Local video recording of working system
- [ ] Screenshots of dashboard
- [ ] Logs showing successful operations
- [ ] Architecture diagram printout

---

## Test Results Summary

| Category | Pass | Fail | Skipped |
|----------|------|------|---------|
| Build Verification | | | |
| MISRA-C Compliance | | | |
| Hardware - Node A | | | |
| Hardware - Node B | | | |
| Communication | | | |
| FreeRTOS Tasks | | | |
| Cloud Server | | | |
| Web Dashboard | | | |
| Integration | | | |
| Performance | | | |
| Security | | | |
| Failure Modes | | | |
| **TOTAL** | | | |

---

**Tested by**: ___________________
**Date**: ___________________
**Notes**:
