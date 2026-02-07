#!/bin/bash

echo "================================================"
echo "  Getting MAC Addresses from ESP32 Devices"
echo "================================================"
echo ""
echo "This script will read the MAC addresses from both"
echo "ESP32 devices and help you update encryption_keys.h"
echo ""

# Node A
echo "Reading Node A MAC address..."
echo "Connect Node A to /dev/ttyUSB0 and press Enter"
read -r

if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
    . "$HOME/esp/esp-idf/export.sh"
fi

cd "$(dirname "$0")/../node-a-gateway"
idf.py -p /dev/ttyUSB0 monitor | grep -m 1 "Base MAC Address"

echo ""
echo "Node A MAC address shown above"
echo ""

# Node B
echo "Reading Node B MAC address..."
echo "Connect Node B to /dev/ttyUSB1 and press Enter"
read -r

cd "../node-b-sentry"
idf.py -p /dev/ttyUSB1 monitor | grep -m 1 "Base MAC Address"

echo ""
echo "Node B MAC address shown above"
echo ""
echo "Update shared/encryption_keys.h with these addresses!"
