# 🔐 RFID Entry Logger Server Setup

## 🚀 Quick Start (3 steps!)

### 1. Install Dependencies
```bash
cd /Users/vrushtee/esp_medmonitor
npm install
```

### 2. Start Server
```bash
npm start
```

You should see:
```
========================================
  🔐 RFID Entry Logger Server
========================================
✅ Server running on http://localhost:3000
📁 Data file: /Users/vrushtee/esp_medmonitor/rfid_entries.json

Ready to log RFID entries! 🚀
========================================
```

### 3. Open Dashboard
```bash
open dashboard.html
```

Connect to MQTT and watch RFID entries get saved! ✅

---

## 📊 What It Does

**Before (without server):**
- RFID entries only in browser memory
- Lost when you refresh page
- Can't access from other devices

**After (with server):**
- ✅ All entries saved to `rfid_entries.json`
- ✅ Persistent across page refreshes
- ✅ Access from multiple devices
- ✅ Full history tracking
- ✅ Statistics and analytics

---

## 🎯 Features

### 1. Automatic Storage
Every RFID scan is automatically saved to disk:
```json
{
  "entries": [
    {
      "id": 1707334567890,
      "uid": "72:0B:A7:05",
      "person": {
        "name": "Authorized Person 1",
        "authorized": true,
        "emoji": "👤"
      },
      "time": "2:30:45 PM",
      "timestamp": "2024-02-07T14:30:45.123Z",
      "date": "2/7/2024",
      "fullTime": "2/7/2024, 2:30:45 PM"
    }
  ]
}
```

### 2. Dashboard Features
- **Live Updates**: See entries as they happen
- **View All**: Button to see complete history
- **Stats**: View access statistics
- **Server Status**: Shows if server is connected

### 3. RESTful API
Full API for custom integrations!

---

## 🌐 API Endpoints

### Get All Entries
```bash
curl http://localhost:3000/api/entries
```

### Get Last 10 Entries
```bash
curl http://localhost:3000/api/entries?limit=10
```

### Add Entry (Manual)
```bash
curl -X POST http://localhost:3000/api/entries \
  -H "Content-Type: application/json" \
  -d '{
    "uid": "AA:BB:CC:DD",
    "person": {"name": "Test User", "authorized": true, "emoji": "👤"},
    "time": "3:00:00 PM"
  }'
```

### Get Statistics
```bash
curl http://localhost:3000/api/stats
```

Response:
```json
{
  "success": true,
  "stats": {
    "total": 25,
    "authorized": 20,
    "unauthorized": 5,
    "byPerson": {
      "Authorized Person 1": 15,
      "Unauthorized Person": 5
    },
    "byDate": {
      "2/7/2024": 25
    }
  }
}
```

### Delete Entry
```bash
curl -X DELETE http://localhost:3000/api/entries/1707334567890
```

### Clear All Entries
```bash
curl -X DELETE "http://localhost:3000/api/entries?confirm=yes"
```

### Health Check
```bash
curl http://localhost:3000/health
```

---

## 📁 File Structure

```
esp_medmonitor/
├── rfid_server.js          ← Server code
├── package.json            ← Dependencies
├── rfid_entries.json       ← Data storage (auto-created)
├── dashboard.html          ← Updated with server integration
└── SERVER_SETUP.md         ← This file
```

---

## 🔧 Configuration

### Change Port
Edit `rfid_server.js`:
```javascript
const PORT = 3000; // Change to any port
```

### Change Data File Location
```javascript
const DATA_FILE = path.join(__dirname, 'rfid_entries.json');
// Change to: '/path/to/your/custom/location.json'
```

### Add More RFID Cards
Edit `dashboard.html`:
```javascript
const rfidDatabase = {
    '72:0B:A7:05': { name: 'Alice', authorized: true, emoji: '👩' },
    '52:81:A2:5C': { name: 'Bob', authorized: false, emoji: '👨' },
    'AA:BB:CC:DD': { name: 'Charlie', authorized: true, emoji: '🧔' }
};
```

---

## 🐛 Troubleshooting

### Server won't start
```bash
# Check if port 3000 is already in use
lsof -i :3000

# Kill process using port 3000
kill -9 <PID>

# Or change port in rfid_server.js
```

### Dashboard shows "Server offline"
```bash
# Make sure server is running
npm start

# Check if it's actually running
curl http://localhost:3000/health
# Should return: {"status":"ok",...}
```

### Entries not saving
1. Check server console for errors
2. Check browser console (F12) for errors
3. Verify `rfid_entries.json` has write permissions

### Can't access from other device
```bash
# Get your local IP
ifconfig | grep "inet "

# Update dashboard.html:
const SERVER_URL = 'http://192.168.1.XXX:3000';

# Make sure firewall allows port 3000
```

---

## 🚀 Advanced Usage

### Run in Background
```bash
# Using nohup
nohup npm start > server.log 2>&1 &

# Using PM2 (process manager)
npm install -g pm2
pm2 start rfid_server.js --name rfid-server
pm2 save
pm2 startup  # Auto-start on boot
```

### View Logs
```bash
# If using nohup
tail -f server.log

# If using PM2
pm2 logs rfid-server
```

### Auto-Restart on Changes (Development)
```bash
npm install -g nodemon
nodemon rfid_server.js
```

---

## 📊 Data Export

### Export to CSV
```bash
node -e "
const fs = require('fs');
const data = JSON.parse(fs.readFileSync('rfid_entries.json'));
console.log('ID,UID,Name,Authorized,Time');
data.entries.forEach(e => {
  console.log(\`\${e.id},\${e.uid},\${e.person.name},\${e.person.authorized},\${e.fullTime}\`);
});
" > entries.csv
```

### Backup Data
```bash
cp rfid_entries.json rfid_entries_backup_$(date +%Y%m%d).json
```

---

## 🎉 Testing

### Test Full Flow
1. Start server: `npm start`
2. Open dashboard: `open dashboard.html`
3. Connect to MQTT
4. Trigger RFID scan from Node B
5. Check dashboard - entry should appear
6. Click "View All" - see full history
7. Click "Stats" - see statistics
8. Refresh page - data persists! ✅

### Manual Test Entry
```bash
curl -X POST http://localhost:3000/api/entries \
  -H "Content-Type: application/json" \
  -d '{
    "uid": "TEST:TEST",
    "person": {"name": "Test Entry", "authorized": true, "emoji": "🧪"},
    "time": "Now"
  }'
```

---

## 🔒 Security Notes

- Server runs on localhost by default (not accessible from internet)
- No authentication on API (add if exposing to network)
- Data stored in plain JSON (encrypt if storing sensitive info)
- Use HTTPS if accessing remotely

---

## 📈 Future Enhancements

Ideas to extend:
- [ ] User authentication
- [ ] Database support (SQLite, MongoDB)
- [ ] Email alerts on unauthorized access
- [ ] Web admin panel
- [ ] CSV export endpoint
- [ ] Real-time WebSocket updates
- [ ] Search and filter entries
- [ ] Automatic backups

---

**Easy, right?** 🎯

Just `npm install && npm start` and you're logging! 🚀
