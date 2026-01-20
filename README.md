# termi

A native ARM64 Linux terminal emulator for iOS. Run Alpine Linux userspace applications directly on your iPhone or iPad with full syscall translation and virtual filesystem support.

## Overview

termi brings a complete Linux userspace environment to iOS by emulating ARM64 CPU instructions, translating Linux system calls to iOS/Darwin equivalents, and providing a SQLite-backed virtual filesystem. Unlike traditional terminal apps that connect to remote servers, termi executes Linux binaries locally with native performance characteristics.

## Features

- **ARM64 CPU Emulation**: Full usermode emulator supporting essential ARMv8-A instruction subset
- **Linux Syscall Translation**: Transparent mapping of 30+ Linux syscalls to iOS sandbox-safe operations
- **Virtual Filesystem**: SQLite-backed storage with /proc, /sys, /dev emulation
- **Alpine Linux Integration**: Complete Alpine Linux rootfs support with package management
- **SwiftUI Terminal**: Modern terminal interface with VT100/ANSI escape sequence support
- **Advanced UI**: Split panes, tab management, customizable themes, keyboard accessories
- **Local Execution**: No remote servers required—all execution happens on-device
- **Sideloading Ready**: Configured for installation via Xcode, AltStore, or TrollStore

## Installation

### Prerequisites

- macOS with Xcode 14+
- iOS device running iOS 16+
- Apple Developer account (free tier sufficient)

### Build from Source

```bash
# Clone repository
git clone https://github.com/yourusername/termi.git
cd termi

# Build native libraries
cd Kernel/syscall && make && cd ../..
cd Filesystem && make && cd ..

# Set up Alpine Linux rootfs
cd Alpine
./setup_alpine.sh
cd ..

# Build iOS app
./Scripts/build.sh

# Open in Xcode
open termi.xcodeproj
```

### Sideload to Device

```bash
# Build and sign for sideloading
xcodebuild -project termi.xcodeproj -scheme termi -configuration Release -destination generic/platform=iOS archive

# Sign with your certificate
./Scripts/sign_app.sh ./build/Release-iphoneos/termi.app "Apple Development: Your Name"

# Install via Xcode, AltStore, or your preferred sideloading method
```

## Usage

```bash
# Launch app on device
# Terminal opens with Alpine Linux shell

# Basic commands work as expected
ls /bin
cat /etc/os-release
apk update
apk add python3

# Python example
python3 -c "print('Hello from ARM64 emulation')"

# File operations persist in virtual filesystem
echo "test" > /tmp/file.txt
cat /tmp/file.txt
```

The terminal supports standard VT100 sequences, split panes (swipe gestures), and multiple tabs. Access settings via the gear icon for theme customization and advanced options.

## Architecture

termi consists of four integrated subsystems:

### Emulator Layer (`Emulator/`)
- **CPU**: ARM64 register state, instruction decoder, interpreter loop
- **MMU**: Virtual memory management with 4KB page tables
- **Loader**: ELF64 binary parsing and memory mapping

### Syscall Layer (`Kernel/`)
- Translates Linux syscalls (open, read, write, etc.) to iOS-safe operations
- Maps ARM64 syscall convention (x8=syscall number) to Darwin equivalents
- Handles privilege restrictions within iOS sandbox

### Filesystem Layer (`Filesystem/`)
- SQLite database stores file metadata (paths, inodes, permissions)
- Blob storage for file contents in `rootfs/data/`
- Virtual filesystems: /proc (CPU info), /sys (kernel parameters), /dev (null, urandom)

### UI Layer (`App/`, `Terminal/`)
- SwiftUI-based terminal with SwiftTerm integration
- PTY handling for bidirectional I/O between terminal and emulator
- Theme system, split views, tab management, keyboard accessories

**Data flow**: Terminal UI → PTY → Emulator executes ARM64 binary → Syscall translation → Filesystem operations → SQLite/blob storage

Full architecture details in [docs/INTEGRATION.md](docs/INTEGRATION.md).

## Development

### Build Libraries

```bash
# Syscall layer
cd Kernel/syscall
make clean && make

# Filesystem layer
cd ../../Filesystem
make clean && make

# Emulator layer (if working on native components)
cd ../Emulator
make clean && make
```

### Project Structure

- `App/` - SwiftUI application code
- `Bridge/` - Swift/C interop layer
- `Emulator/` - ARM64 CPU emulation (C)
- `Kernel/` - Linux syscall translation (C)
- `Filesystem/` - Virtual filesystem (C + SQLite)
- `Alpine/` - Alpine Linux rootfs setup scripts
- `Terminal/` - Terminal UI components (Swift)
- `docs/` - Detailed component documentation

### Requirements

- Xcode 14+ (Swift 5.9+, iOS 16+ deployment target)
- Dependencies: SwiftTerm (via Swift Package Manager), SQLite3 (system library)
- Build tools: make, clang, swift

### Testing

```bash
# Run unit tests
xcodebuild test -project termi.xcodeproj -scheme termi -destination 'platform=iOS Simulator,name=iPhone 15'

# Manual integration test
# 1. Build and run on device
# 2. Execute: /bin/sh -c 'echo test > /tmp/file && cat /tmp/file'
# 3. Verify output matches input
```

## Documentation

Comprehensive documentation available in `docs/`:

- [INTEGRATION.md](docs/INTEGRATION.md) - System architecture and component integration
- [EMULATOR_STATUS.md](docs/EMULATOR_STATUS.md) - ARM64 emulation implementation
- [FILESYSTEM_STATUS.md](docs/FILESYSTEM_STATUS.md) - Virtual filesystem design
- [SYSCALL_STATUS.md](docs/SYSCALL_STATUS.md) - Linux syscall translation
- [TERMINAL_UI_STATUS.md](docs/TERMINAL_UI_STATUS.md) - SwiftUI terminal implementation
- [BUILD_STATUS.md](docs/BUILD_STATUS.md) - Build configuration and status

## License

MIT License
