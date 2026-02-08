/**
 * Simple RFID Entry Logger Server
 * Stores RFID access logs with timestamps
 */

const express = require('express');
const fs = require('fs');
const path = require('path');
const cors = require('cors');

const app = express();
const PORT = 3000;
const DATA_FILE = path.join(__dirname, 'rfid_entries.json');

// Middleware
app.use(cors()); // Allow requests from dashboard
app.use(express.json());

// Initialize data file if it doesn't exist
if (!fs.existsSync(DATA_FILE)) {
    fs.writeFileSync(DATA_FILE, JSON.stringify({ entries: [] }));
    console.log('✅ Created rfid_entries.json');
}

// Load entries from file
function loadEntries() {
    try {
        const data = fs.readFileSync(DATA_FILE, 'utf8');
        return JSON.parse(data).entries;
    } catch (error) {
        console.error('Error loading entries:', error);
        return [];
    }
}

// Save entries to file
function saveEntries(entries) {
    try {
        fs.writeFileSync(DATA_FILE, JSON.stringify({ entries }, null, 2));
        return true;
    } catch (error) {
        console.error('Error saving entries:', error);
        return false;
    }
}

// ============================================================
// API Endpoints
// ============================================================

// GET /api/entries - Get all RFID entries (with optional limit)
app.get('/api/entries', (req, res) => {
    const entries = loadEntries();
    const limit = parseInt(req.query.limit) || entries.length;

    // Return most recent entries first
    const limitedEntries = entries.slice(-limit).reverse();

    console.log(`📖 GET /api/entries - Returning ${limitedEntries.length} entries`);
    res.json({
        success: true,
        count: limitedEntries.length,
        total: entries.length,
        entries: limitedEntries
    });
});

// POST /api/entries - Add new RFID entry
app.post('/api/entries', (req, res) => {
    const { uid, person, time } = req.body;

    if (!uid || !person || !time) {
        console.log('❌ POST /api/entries - Missing required fields');
        return res.status(400).json({
            success: false,
            error: 'Missing required fields: uid, person, time'
        });
    }

    const entries = loadEntries();

    const newEntry = {
        id: Date.now(), // Unique ID
        uid,
        person,
        time,
        timestamp: new Date().toISOString(),
        date: new Date().toLocaleDateString(),
        fullTime: new Date().toLocaleString()
    };

    entries.push(newEntry);

    if (saveEntries(entries)) {
        console.log(`✅ POST /api/entries - Added entry for ${person.name} (${uid})`);
        res.json({
            success: true,
            entry: newEntry,
            total: entries.length
        });
    } else {
        console.log('❌ POST /api/entries - Failed to save');
        res.status(500).json({
            success: false,
            error: 'Failed to save entry'
        });
    }
});

// GET /api/stats - Get entry statistics
app.get('/api/stats', (req, res) => {
    const entries = loadEntries();

    // Count by person
    const byPerson = {};
    const byDate = {};
    let authorized = 0;
    let unauthorized = 0;

    entries.forEach(entry => {
        // By person
        const name = entry.person.name;
        byPerson[name] = (byPerson[name] || 0) + 1;

        // By date
        const date = entry.date;
        byDate[date] = (byDate[date] || 0) + 1;

        // By authorization
        if (entry.person.authorized) {
            authorized++;
        } else {
            unauthorized++;
        }
    });

    console.log('📊 GET /api/stats - Returning statistics');
    res.json({
        success: true,
        stats: {
            total: entries.length,
            authorized,
            unauthorized,
            byPerson,
            byDate,
            firstEntry: entries[0]?.fullTime || null,
            lastEntry: entries[entries.length - 1]?.fullTime || null
        }
    });
});

// DELETE /api/entries/:id - Delete specific entry
app.delete('/api/entries/:id', (req, res) => {
    const id = parseInt(req.params.id);
    const entries = loadEntries();

    const index = entries.findIndex(e => e.id === id);

    if (index === -1) {
        console.log(`❌ DELETE /api/entries/${id} - Entry not found`);
        return res.status(404).json({
            success: false,
            error: 'Entry not found'
        });
    }

    const deleted = entries.splice(index, 1)[0];

    if (saveEntries(entries)) {
        console.log(`✅ DELETE /api/entries/${id} - Deleted entry for ${deleted.person.name}`);
        res.json({
            success: true,
            deleted,
            remaining: entries.length
        });
    } else {
        res.status(500).json({
            success: false,
            error: 'Failed to save after deletion'
        });
    }
});

// DELETE /api/entries - Clear all entries (with confirmation)
app.delete('/api/entries', (req, res) => {
    const { confirm } = req.query;

    if (confirm !== 'yes') {
        return res.status(400).json({
            success: false,
            error: 'Add ?confirm=yes to clear all entries'
        });
    }

    const entries = loadEntries();
    const count = entries.length;

    if (saveEntries([])) {
        console.log(`🗑️  DELETE /api/entries - Cleared ${count} entries`);
        res.json({
            success: true,
            message: `Cleared ${count} entries`,
            cleared: count
        });
    } else {
        res.status(500).json({
            success: false,
            error: 'Failed to clear entries'
        });
    }
});

// Health check
app.get('/health', (req, res) => {
    res.json({
        status: 'ok',
        uptime: process.uptime(),
        timestamp: new Date().toISOString()
    });
});

// ============================================================
// Start Server
// ============================================================

app.listen(PORT, () => {
    console.log('');
    console.log('========================================');
    console.log('  🔐 RFID Entry Logger Server');
    console.log('========================================');
    console.log(`✅ Server running on http://localhost:${PORT}`);
    console.log(`📁 Data file: ${DATA_FILE}`);
    console.log('');
    console.log('API Endpoints:');
    console.log(`  GET    /api/entries       - Get all entries`);
    console.log(`  GET    /api/entries?limit=10 - Get last 10 entries`);
    console.log(`  POST   /api/entries       - Add new entry`);
    console.log(`  GET    /api/stats         - Get statistics`);
    console.log(`  DELETE /api/entries/:id   - Delete specific entry`);
    console.log(`  DELETE /api/entries?confirm=yes - Clear all`);
    console.log(`  GET    /health            - Health check`);
    console.log('');
    console.log('Ready to log RFID entries! 🚀');
    console.log('========================================');
});

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\n👋 Shutting down server...');
    process.exit(0);
});
