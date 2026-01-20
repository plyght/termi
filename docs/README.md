# termi Documentation

Documentation for the termi ARM64 Linux terminal emulator for iOS.

## Architecture Overview

**[INTEGRATION.md](INTEGRATION.md)** - Complete system architecture and component integration guide

## Component Documentation

### Core Systems

- **[EMULATOR_STATUS.md](EMULATOR_STATUS.md)** - ARM64 CPU emulator implementation
  - Instruction decoder, interpreter, MMU
  - ELF loader, syscall interface
  - Performance characteristics

- **[FILESYSTEM_STATUS.md](FILESYSTEM_STATUS.md)** - Virtual filesystem layer
  - SQLite-backed storage
  - /proc, /sys, /dev emulation
  - API reference: [FILESYSTEM_API.md](FILESYSTEM_API.md)

- **[SYSCALL_STATUS.md](SYSCALL_STATUS.md)** - Linux syscall translation
  - iOS/Darwin syscall mapping
  - Supported syscalls and limitations
  - Integration: [KERNEL_INTEGRATION.md](KERNEL_INTEGRATION.md)

### User Interface

- **[TERMINAL_UI_STATUS.md](TERMINAL_UI_STATUS.md)** - SwiftUI terminal interface
  - VT100/ANSI terminal emulation
  - Theme system, split panes, tab management
  - Full implementation: [TERMINAL_UI_IMPLEMENTATION.md](TERMINAL_UI_IMPLEMENTATION.md)

### Build & Deployment

- **[BUILD_STATUS.md](BUILD_STATUS.md)** - Xcode project setup and build configuration
  - Project structure
  - Code signing for sideloading
  - Dependencies and build steps

## Quick Navigation

| Component | Status File | Implementation Details |
|-----------|-------------|------------------------|
| ARM64 Emulator | [EMULATOR_STATUS.md](EMULATOR_STATUS.md) | Core CPU emulation |
| Virtual Filesystem | [FILESYSTEM_STATUS.md](FILESYSTEM_STATUS.md) | [FILESYSTEM_API.md](FILESYSTEM_API.md) |
| Syscall Layer | [SYSCALL_STATUS.md](SYSCALL_STATUS.md) | [KERNEL_INTEGRATION.md](KERNEL_INTEGRATION.md) |
| Terminal UI | [TERMINAL_UI_STATUS.md](TERMINAL_UI_STATUS.md) | [TERMINAL_UI_IMPLEMENTATION.md](TERMINAL_UI_IMPLEMENTATION.md) |
| Build System | [BUILD_STATUS.md](BUILD_STATUS.md) | Xcode configuration |

## Getting Started

1. Read [INTEGRATION.md](INTEGRATION.md) to understand how all components fit together
2. Check individual component status files for implementation details
3. Refer to API documentation for filesystem and syscall interfaces
4. Review build status for Xcode project configuration
