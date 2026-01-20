#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT"

echo "Building libsyscall.a..."
cd Kernel/syscall
make clean
make
cd "$PROJECT_ROOT"

echo "Building libfakefs.a..."
cd Filesystem
make clean
make
cd "$PROJECT_ROOT"

echo "Building Xcode project..."
xcodebuild -project termi.xcodeproj \
    -scheme termi \
    -destination 'generic/platform=iOS' \
    -configuration Release \
    clean build \
    CODE_SIGN_IDENTITY="" \
    CODE_SIGNING_REQUIRED=NO \
    CODE_SIGNING_ALLOWED=NO

echo "Build complete!"
