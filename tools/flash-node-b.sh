#!/bin/bash
set -e

echo "================================================"
echo "  Flashing Node B (Sentry)"
echo "================================================"

cd "$(dirname "$0")/../node-b-sentry"

# Source ESP-IDF environment
if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
    . "$HOME/esp/esp-idf/export.sh"
elif [ -n "$IDF_PATH" ]; then
    . "$IDF_PATH/export.sh"
else
    echo "Error: ESP-IDF not found"
    echo "Please install ESP-IDF and set IDF_PATH"
    exit 1
fi

# Build
echo "Building Node B..."
idf.py build

# Flash and monitor (change port as needed)
echo "Flashing to /dev/ttyUSB1..."
idf.py -p /dev/ttyUSB1 flash monitor
