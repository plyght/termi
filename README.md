# termi

ARM64 Linux Terminal for iOS 26 - Run Alpine Linux binaries natively on iOS devices.

## Overview

termi is a full-featured ARM64 Linux emulator for iOS that allows you to run Alpine Linux in a native iOS app.

## Features

- ARM64 CPU emulation with JIT compilation
- Linux syscall translation to iOS/Darwin
- SQLite-backed virtual filesystem
- SwiftUI terminal interface with VT100/ANSI support
- Alpine Linux userspace environment

## Requirements

- Xcode 26.0 or later
- iOS 16.0+ deployment target
- ARM64 iOS device
- macOS for building

## Quick Start

```bash
cd Kernel/syscall && make
cd ../../Filesystem && make
open termi.xcodeproj
```

Build and run on your iOS device.

## Documentation

See `INTEGRATION.md` for architecture details.

## License

MIT
