# 🔐 RFID Dashboard Test Guide

## Issue Found & Fixed! ✅

**Problem:** Dashboard wasn't subscribed to `rfid/scanned` topic!

**Solution:** Added `mqttClient.subscribe('rfid/#')` to dashboard

---

## 🧪 How to Test

### Step 1: Flash Node B
```bash
cd node_b
idf.py build flash monitor
```

Wait for:
```
✅ MQTT CONNECTED!
✅ ESP-NOW READY
RC522 RFID reader initialized
```

### Step 2: Start Server (Optional - for storage)
```bash
cd /Users/vrushtee/esp_medmonitor
npm start
```

### Step 3: Open Dashboard
```bash
open dashboard.html
```

1. Enter MQTT credentials:
   - Broker: `79b8ce293de041719ad94f0b3b8ce5c7.s1.eu.hivemq.cloud`
   - Username: `vruga`
   - Password: `Vrushtee@512`

2. Click "Connect to MQTT"

3. Open browser console (F12 or Cmd+Option+J) to see debug logs

### Step 4: Scan RFID Card

**Expected UIDs in Node B:**
- **Authorized**: `72:0B:A7:05` → "Authorized Tag 1"
- **Unauthorized**: `52:81:A2:5C` → "Unauthorized Tag 1"

---

## ✅ What You Should See

### In Node B Monitor:
```
I (12345) SECURITY_SYSTEM: RFID Card detected - UID: 72:0B:A7:05
I (12346) SECURITY_SYSTEM: ✓ AUTHORIZED - Authorized Tag 1
I (12347) SECURITY_SYSTEM: 📡 MQTT -> rfid/scanned: 72:0B:A7:05
```

### In Browser Console (F12):
```
📱 RFID Scanned: 72:0B:A7:05
👤 Person: {name: 'Authorized Person 1', authorized: true, emoji: '👤'}
✅ Saved to server: {...}
```

### On Dashboard:
A new entry appears in the RFID Access Log:
```
┌────────────────────────────────────────┐
│ ✅ Authorized Person 1                 │
│    RFID: 72:0B:A7:05                   │
│    2:45:30 PM                          │
└────────────────────────────────────────┘
```

**Sound:** 800Hz beep (authorized) or 400Hz beep (unauthorized)

---

## 🐛 Troubleshooting

### Dashboard shows nothing
1. **Check browser console (F12):**
   ```
   ✅ Connected to MQTT broker
   ✅ Subscribed to: node_a/#, security/#, rfid/#
   ```

2. **Verify MQTT connection:**
   - Status bar should show green dot "Connected"
   - If red, check credentials

3. **Watch for RFID message:**
   - When you scan, console should show: `📱 RFID Scanned: XX:XX:XX:XX`
   - If nothing, check Node B is publishing

### Node B not publishing
Check Node B monitor:
```bash
cd node_b && idf.py monitor
```

Look for:
```
✅ MQTT CONNECTED!
📡 MQTT -> rfid/scanned: XX:XX:XX:XX
```

If "MQTT DISCONNECTED", check:
- WiFi credentials in Node B
- HiveMQ Cloud credentials
- Internet connection

### UID doesn't match
Node B publishes UID like: `72:0B:A7:05`

Dashboard expects same format in database:
```javascript
const rfidDatabase = {
    '72:0B:A7:05': { ... }  // Must match exactly!
};
```

**Case sensitive? NO** - but separator must be `:` not `-`

### "Unknown Person" appears
This means the UID isn't in the dashboard database.

**To add your card:**
1. Scan it and note the UID from console
2. Edit `dashboard.html`:
   ```javascript
   const rfidDatabase = {
       '72:0B:A7:05': { name: 'Alice', authorized: true, emoji: '👩' },
       '52:81:A2:5C': { name: 'Bob', authorized: false, emoji: '👨' },
       'YOUR:UID:HERE': { name: 'Your Name', authorized: true, emoji: '😊' }
   };
   ```
3. Refresh dashboard
4. Scan again - should show your name!

---

## 🎯 Quick Debug Checklist

- [ ] Node B monitor shows "MQTT CONNECTED"
- [ ] Dashboard status shows green "Connected"
- [ ] Browser console shows "Subscribed to: node_a/#, security/#, rfid/#"
- [ ] Scan RFID card
- [ ] Node B monitor shows "RFID Card detected"
- [ ] Node B monitor shows "📡 MQTT -> rfid/scanned"
- [ ] Browser console shows "📱 RFID Scanned"
- [ ] Entry appears on dashboard
- [ ] Sound plays
- [ ] Server shows "✅ Saved to server" (if running)

---

## 🔥 Still Not Working?

### Test MQTT directly:
Use MQTT Explorer or mosquitto_sub to see if messages are publishing:

```bash
# Install mosquitto client
brew install mosquitto

# Subscribe to all topics
mosquitto_sub -h 79b8ce293de041719ad94f0b3b8ce5c7.s1.eu.hivemq.cloud \
  -p 8883 \
  -u vruga \
  -P Vrushtee@512 \
  --cafile /path/to/ca-cert.pem \
  -t '#' \
  -v
```

Scan RFID → Should see:
```
rfid/scanned 72:0B:A7:05
```

### Check HiveMQ Cloud Dashboard:
1. Go to https://console.hivemq.cloud/
2. Login
3. View your cluster
4. Check "Web Client" tab
5. Subscribe to `rfid/#`
6. Scan RFID
7. Should see message appear

---

## 📝 Summary of Changes

**What was broken:**
```javascript
// Old - missing rfid/# subscription
mqttClient.subscribe('node_a/#');
mqttClient.subscribe('security/#');
// ❌ RFID messages not received!
```

**What I fixed:**
```javascript
// New - added rfid/# subscription
mqttClient.subscribe('node_a/#');
mqttClient.subscribe('security/#');
mqttClient.subscribe('rfid/#');  // ✅ Now receives RFID!
```

**Why it's important:**
- MQTT uses topic patterns for pub/sub
- `node_a/#` matches `node_a/sensors`, `node_a/anything`
- `security/#` matches `security/alarm`, `security/status`
- `rfid/#` matches `rfid/scanned` ← **This was missing!**

---

**Try it now and let me know if entries appear!** 🎯
