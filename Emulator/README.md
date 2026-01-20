# ARM64 Usermode Emulator

High-performance ARM64 userspace emulator for termi iOS Terminal app.

## Architecture

```
Emulator/
├── cpu/                    # CPU emulation core
│   ├── arm64_cpu.h/c      # CPU state & register management
│   ├── decoder.h/c        # ARM64 instruction decoder
│   ├── interpreter.c      # Instruction execution engine
│   └── exceptions.c       # Exception handling (TODO)
├── mmu/                    # Memory management
│   ├── mmu.h/c            # Virtual memory manager
│   └── tlb.c              # TLB simulation (TODO)
├── loader/                 # Binary loading
│   ├── elf_loader.h/c     # ELF64 parser & loader
│   └── dynamic_linker.c   # Dynamic linker (TODO)
├── emulator.h/c           # Public API
└── Makefile               # Build system
```

## Features

### Implemented

- ✅ ARM64 CPU state (X0-X30, SP, PC, PSTATE)
- ✅ FP/SIMD registers (V0-V31)
- ✅ Essential ARMv8-A instruction decoder
- ✅ Instruction interpreter loop
- ✅ Virtual memory management (4KB pages)
- ✅ ELF64 binary loader
- ✅ Syscall interface integration
- ✅ Stack management

### Supported Instructions

- Data processing: ADD, SUB, AND, ORR, EOR, MUL, DIV
- Load/Store: LDR, STR, LDP, STP (immediate/register)
- Branches: B, BL, BR, BLR, RET, B.cond, CBZ, CBNZ
- Moves: MOV, MOVZ, MOVK, MOVN
- Comparisons: CMP, TST
- Address computation: ADRP, ADR
- System: SVC (syscall), NOP

### TODO

- ❌ Advanced SIMD/FP instructions
- ❌ Atomic operations
- ❌ TLB caching optimization
- ❌ Dynamic linker support
- ❌ Thread-local storage (TLS)
- ❌ Signal handling

## Integration with termi

### Syscall Interface

When ARM64 code executes `SVC #0`, the emulator calls:

```c
long handle_syscall(int num, long arg1, long arg2, long arg3,
                   long arg4, long arg5, long arg6);
```

Located at `/Users/nicojaffer/termi/Kernel/syscall/calls.h`

### Memory Layout

```
0x00000000 - 0x00010000  : ELF text segment
0x00010000 - 0x00020000  : ELF data segment
0x40000000 - 0x50000000  : Heap (brk)
0x50000000 - 0x60000000  : mmap region
0x7FFFFFFF0000 - 0x7FFFFFFFF000 : Stack (8MB)
```

### Example Usage

```c
#include "Emulator/emulator.h"

arm64_emulator_t *emu = arm64_emulator_create();

if (arm64_emulator_load_elf(emu, "/path/to/binary") < 0) {
    fprintf(stderr, "Failed to load ELF\n");
    return 1;
}

arm64_emulator_run(emu);

printf("Exit code: %d\n", emu->cpu->exit_code);

arm64_emulator_destroy(emu);
```

## Building

```bash
cd /Users/nicojaffer/termi/Emulator
make clean
make
```

Produces: `libemulator.a`

## Performance

Target: 70-80% of native ARM64 performance on Apple Silicon.

Optimizations:

- Direct-threaded interpreter
- Hash-based page table lookups
- Minimal bounds checking (usermode only)
- No JIT compilation (iOS restriction)

## Testing

```bash
make test
./test_emulator
```

## References

- ARM Architecture Reference Manual ARMv8
- QEMU user-mode emulation
- Unicorn Engine API design
- Box64 ARM64 userspace patterns
- iSH x86 emulator for iOS

## Integration Points

### Agent 3 (Terminal UI)

Call emulator from PTY handler:

```c
arm64_emulator_run(global_emu);
```

### Agent 4 (Syscall Layer)

Already integrated via `handle_syscall()`.

### Agent 5 (Filesystem)

Accessed automatically through syscalls.

## License

Part of termi project.
