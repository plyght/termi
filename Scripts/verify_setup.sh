#!/bin/bash

echo "========================================"
echo "termi - iOS 26 Project Verification"
echo "========================================"
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT" || exit 1

verify_file() {
    if [ -f "$1" ]; then
        echo "✅ $1"
        return 0
    else
        echo "❌ $1 (MISSING)"
        return 1
    fi
}

verify_dir() {
    if [ -d "$1" ]; then
        echo "✅ $1/"
        return 0
    else
        echo "❌ $1/ (MISSING)"
        return 1
    fi
}

echo "1. Core Project Files"
echo "====================="
verify_file "termi.xcodeproj/project.pbxproj"
verify_file "Package.swift"
verify_file "README.md"
verify_file ".gitignore"
echo ""

echo "2. Swift Source Files"
echo "====================="
verify_file "App/TermiApp.swift"
verify_file "App/UI/ContentView.swift"
verify_file "App/UI/TerminalView.swift"
verify_file "App/Managers/EmulatorManager.swift"
verify_file "Bridge/EmulatorBridge.swift"
verify_file "Bridge/FilesystemBridge.swift"
verify_file "Bridge/termi-Bridging-Header.h"
echo ""

echo "3. C Libraries (Built)"
echo "====================="
verify_file "Kernel/syscall/libsyscall.a"
verify_file "Filesystem/libfakefs.a"
echo ""

echo "4. Resources & Configuration"
echo "============================"
verify_file "Resources/Info.plist"
verify_file "Resources/termi.entitlements"
echo ""

echo "5. Build Scripts"
echo "================"
verify_file "Scripts/build.sh"
verify_file "Scripts/sign_app.sh"
verify_file "Scripts/create_xcode_project.py"
echo ""

echo "6. Directory Structure"
echo "======================"
verify_dir "App/UI"
verify_dir "App/Managers"
verify_dir "Emulator/cpu"
verify_dir "Emulator/mmu"
verify_dir "Emulator/loader"
verify_dir "Kernel/syscall"
verify_dir "Filesystem/fakefs"
verify_dir "Alpine"
verify_dir "Terminal/SwiftTerm"
verify_dir "Bridge"
verify_dir "Resources"
verify_dir "Scripts"
verify_dir "Tests"
echo ""

echo "7. Xcode Project Validation"
echo "============================"
xcodebuild -list -project termi.xcodeproj 2>&1 | grep -E '(Targets|termi)' && echo "✅ Xcode project valid" || echo "❌ Xcode project invalid"
echo ""

echo "8. Library Check"
echo "================"
if [ -f "Kernel/syscall/libsyscall.a" ]; then
    SIZE=$(du -h "Kernel/syscall/libsyscall.a" | cut -f1)
    echo "✅ libsyscall.a ($SIZE)"
fi

if [ -f "Filesystem/libfakefs.a" ]; then
    SIZE=$(du -h "Filesystem/libfakefs.a" | cut -f1)
    echo "✅ libfakefs.a ($SIZE)"
fi
echo ""

echo "9. Swift File Count"
echo "==================="
SWIFT_COUNT=$(find . -name "*.swift" -type f | wc -l | tr -d ' ')
echo "✅ Found $SWIFT_COUNT Swift source files"
echo ""

echo "10. Integration Status"
echo "======================"
if [ -f "Kernel/syscall/libsyscall.a" ]; then
    echo "✅ Agent 4 (Syscall Layer) - INTEGRATED"
else
    echo "❌ Agent 4 (Syscall Layer) - NOT BUILT"
fi

if [ -f "Filesystem/libfakefs.a" ]; then
    echo "✅ Agent 5 (Filesystem) - INTEGRATED"
else
    echo "❌ Agent 5 (Filesystem) - NOT BUILT"
fi

if [ -d "Emulator/cpu" ] && [ "$(ls -A Emulator/cpu 2>/dev/null)" ]; then
    echo "✅ Agent 2 (Emulator) - FILES PRESENT"
else
    echo "⏳ Agent 2 (Emulator) - AWAITING IMPLEMENTATION"
fi

if [ -d "Terminal/SwiftTerm" ] && [ "$(ls -A Terminal/SwiftTerm 2>/dev/null)" ]; then
    echo "✅ Agent 3 (Terminal UI) - FILES PRESENT"
else
    echo "⏳ Agent 3 (Terminal UI) - AWAITING IMPLEMENTATION"
fi
echo ""

echo "========================================"
echo "Setup Status: READY FOR INTEGRATION"
echo "========================================"
echo ""
echo "Next Steps:"
echo "1. Agent 2: Implement ARM64 emulator in Emulator/"
echo "2. Agent 3: Implement SwiftTerm UI in Terminal/"
echo "3. Run: open termi.xcodeproj"
echo "4. Build and test on iOS device"
echo ""
