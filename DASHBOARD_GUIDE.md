# 🎨 ESP32 Medical Monitor Dashboard Guide

## 🚀 How to Use

1. **Flash both ESP32s:**
   ```bash
   # Terminal 1 - Node A (Sensors)
   cd node_a
   idf.py build flash monitor

   # Terminal 2 - Node B (Security)
   cd node_b
   idf.py build flash monitor
   ```

2. **Open the dashboard:**
   ```bash
   open dashboard.html
   ```
   Or just double-click `dashboard.html`

3. **Enter MQTT credentials:**
   - Broker: `79b8ce293de041719ad94f0b3b8ce5c7.s1.eu.hivemq.cloud`
   - Username: `vruga`
   - Password: `Vrushtee@512`

4. **Click "Connect to MQTT"** and watch the magic! ✨

---

## 🏗️ How Everything Connects

### The Simple Explanation

```
┌─────────────────────────────────────────────────────────────┐
│                    YOUR SETUP                               │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ESP32 (Node A)          HiveMQ Cloud           Browser     │
│  ├─ Sensors          ←→  ☁️  Broker      ←→   💻 You       │
│  └─ Emergency            (Internet)          (dashboard)    │
│       ║                                                     │
│       ║ ESP-NOW (Direct!)                                  │
│       ║                                                     │
│       ▼                                                     │
│  ESP32 (Node B)                                            │
│  └─ Buzzer + Security                                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### The Detailed Flow

#### 1. **ESP32s Connect to Cloud**
- Both ESP32s connect to your WiFi
- They connect to HiveMQ Cloud MQTT broker using:
  - Protocol: MQTT over TLS (encrypted!)
  - Port: 8883
  - Auth: Username/password

#### 2. **Dashboard Connects to Same Cloud**
- Your browser opens `dashboard.html`
- JavaScript runs in your browser (locally!)
- MQTT.js library connects to HiveMQ Cloud using:
  - Protocol: MQTT over WebSocket (browsers can't use raw TCP)
  - Port: 8884 (WebSocket port)
  - Auth: Same username/password

#### 3. **Real-Time Data Flow**
```
Node A reads sensors (temp, humidity, heart rate, posture)
    ↓
Publishes JSON to topic: node_a/sensors
    ↓
HiveMQ Cloud receives and distributes
    ↓
Your browser (subscribed to node_a/#) gets message
    ↓
Dashboard updates UI with animations!
```

#### 4. **ESP-NOW Emergency System**
When emergency detected (fall, button press, etc.):

**Path 1: MQTT (Normal)**
```
Node A → HiveMQ Cloud → Your Dashboard
        → Shows big red alert!
```

**Path 2: ESP-NOW (Backup)**
```
Node A → ESP-NOW → Node B → Buzzer sounds!
              → Also publishes to MQTT
```

**Why both?** ESP-NOW works even if WiFi dies! It's peer-to-peer radio between ESP32s.

---

## 📡 MQTT Topics

The dashboard subscribes to these topics:

| Topic | Source | Contains |
|-------|--------|----------|
| `node_a/sensors` | Node A | Temperature, humidity, heart rate, posture, emergency_type |
| `security/alarm` | Node B | "INTRUSION" or "OFF" |
| `security/espnow_alert` | Node B | ESP-NOW emergency messages from Node A |
| `security/status` | Node B | Security system state (IDLE, IR_DETECTED, AUTHORIZED, INTRUSION) |
| `rfid/scanned` | Node B | RFID card UIDs |

---

## 🎯 Features

### Visual Indicators
- 🟢 **Green dot** = Connected to MQTT
- 🔴 **Red dot** = Disconnected
- ✨ **Glowing values** = Sensor just updated
- 🚨 **Red banner** = Emergency detected!
- 📡 **ESP-NOW card lights up** = Direct Node A→B communication

### Real-Time Updates
- Temperature & humidity charts (last 20 readings)
- Heart rate monitoring with graphs
- Posture detection (Standing/Sitting/Lying)
- Emergency alerts (Fall, Button, High Temp, High HR, Intrusion)

### Emergency Types
1. **FALL** - MPU6050 detected sudden acceleration
2. **BUTTON** - Emergency button pressed
3. **HIGH_TEMP** - Fever detected
4. **HIGH_HR** - Abnormal heart rate
5. **INTRUSION** - Unauthorized entry (no RFID)

---

## 🔧 Technical Details

### Why WebSocket?
Browsers can't make raw TCP connections for security reasons. WebSocket wraps MQTT in HTTP, making it browser-compatible!

### Why HiveMQ Cloud?
- ✅ Free tier available
- ✅ TLS encryption built-in
- ✅ WebSocket support
- ✅ Accessible from anywhere
- ✅ No need to run local broker

### How is it Secure?
- TLS 1.2+ encryption on all connections
- Username/password authentication
- Certificate validation (ISRG Root X1)
- No open ports on your laptop!

### Can Others See My Dashboard?
- **If file:// protocol**: Only you (local HTML file)
- **If you host it**: Anyone with the URL + MQTT credentials
- **ESP32 data**: Only visible to those with HiveMQ login

---

## 🎨 Dashboard Features

### Orange Theme
- Vibrant orange gradient background
- Glass-morphism cards
- Smooth animations everywhere

### Interactive Elements
- Hover effects on cards
- Pulsing glow on value updates
- Bouncing emoji header
- Shake animation on emergencies
- Progress bars and status indicators

### Charts
- Real-time line graphs
- Orange color palette
- Auto-scrolling (keeps last 20 points)
- Smooth curve interpolation

---

## 🐛 Troubleshooting

### Dashboard won't connect?
1. Check WiFi on both ESP32s
2. Verify HiveMQ credentials
3. Check browser console (F12) for errors
4. Make sure ESP32s are publishing (check monitor logs)

### No data showing?
1. Confirm ESP32s connected to MQTT (look for "✅ MQTT CONNECTED!")
2. Check dashboard status bar shows "Connected"
3. Verify ESP32s are publishing every 5 seconds

### ESP-NOW not working?
1. Verify MAC addresses match in code
2. Both ESP32s on same WiFi channel
3. Check Node B monitor for "📥 ESP-NOW: Received"
4. Distance: ESP-NOW works up to ~100m outdoors

---

## 🎉 Cool Things You Can Do

1. **Multiple Dashboards**: Open on laptop + phone simultaneously!
2. **Remote Monitoring**: Access from anywhere with internet
3. **Data Logging**: HiveMQ stores recent messages
4. **Custom Alerts**: Modify thresholds in Node A code
5. **Theme Changes**: Edit CSS to customize colors

---

## 📚 Learn More

- **MQTT**: Lightweight pub/sub messaging protocol
- **WebSocket**: Bidirectional communication over HTTP
- **ESP-NOW**: Espressif's proprietary peer-to-peer protocol
- **Chart.js**: JavaScript charting library
- **HiveMQ**: MQTT broker hosting service

---

**Made with 🧡 and ESP32s!**
