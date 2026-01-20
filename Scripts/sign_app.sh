#!/bin/bash
set -e

if [ $# -lt 2 ]; then
    echo "Usage: $0 <app_path> <signing_identity>"
    echo "Example: $0 ./build/Release-iphoneos/termi.app \"Apple Development: Your Name (TEAMID)\""
    exit 1
fi

APP_PATH="$1"
SIGNING_IDENTITY="$2"

echo "Signing app at: $APP_PATH"
echo "Using identity: $SIGNING_IDENTITY"

codesign --force --sign "$SIGNING_IDENTITY" \
    --entitlements "$(dirname "$0")/../Resources/termi.entitlements" \
    --deep \
    --timestamp \
    --options runtime \
    "$APP_PATH"

echo "Verifying signature..."
codesign --verify --verbose "$APP_PATH"

echo "App signed successfully!"
echo "You can now sideload with: ideviceinstaller -i $APP_PATH"
