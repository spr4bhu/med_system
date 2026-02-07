#!/bin/bash
# Copies shared protocol files to both ESP-IDF projects

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

echo "Syncing shared protocol files..."

# Node A
TARGET_A="$ROOT_DIR/node-a-gateway/components/shared_protocol/include"
mkdir -p "$TARGET_A"
cp "$SCRIPT_DIR/protocol.h" "$TARGET_A/"
cp "$SCRIPT_DIR/message_types.h" "$TARGET_A/"
if [ -f "$SCRIPT_DIR/encryption_keys.h" ]; then
    cp "$SCRIPT_DIR/encryption_keys.h" "$TARGET_A/"
    echo "  ✓ Copied encryption_keys.h to Node A"
else
    echo "  ⚠ Warning: encryption_keys.h not found (copy from template)"
fi
echo "  ✓ Synced to Node A"

# Node B
TARGET_B="$ROOT_DIR/node-b-sentry/components/shared_protocol/include"
mkdir -p "$TARGET_B"
cp "$SCRIPT_DIR/protocol.h" "$TARGET_B/"
cp "$SCRIPT_DIR/message_types.h" "$TARGET_B/"
if [ -f "$SCRIPT_DIR/encryption_keys.h" ]; then
    cp "$SCRIPT_DIR/encryption_keys.h" "$TARGET_B/"
    echo "  ✓ Copied encryption_keys.h to Node B"
fi
echo "  ✓ Synced to Node B"

echo "✅ Shared protocol synced to both nodes"
