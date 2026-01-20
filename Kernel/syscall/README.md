# Linux → Darwin Syscall Translation Layer

ARM64 Linux syscall translation layer for termi - enables running Alpine Linux binaries on iOS 26.

## Architecture

This syscall translation layer sits between the ARM64 emulator and Darwin (iOS) system calls:

```
Linux Binary → ARM64 Emulator → [SVC Instruction] → handle_syscall() → Darwin APIs
```

## File Structure

```
Kernel/
├── include/
│   └── types.h           - Type definitions and error codes
└── syscall/
    ├── calls.h           - Syscall declarations
    ├── calls.c           - Syscall table and dispatcher
    ├── process.c         - Process management (fork, exec, exit, wait)
    ├── fs.c              - File I/O operations
    ├── memory.c          - Memory management (mmap, brk)
    ├── network.c         - Socket operations
    └── user_access.c     - Guest memory access helpers
```

## Key Features

### Process Management (process.c)

- **fork**: Implemented via posix_spawn (iOS doesn't support real fork)
- **execve**: Full argument/environment translation
- **exit**: Process termination
- **wait4/waitpid**: Process waiting with status
- **clone**: Thread support (basic implementation)
- **kill**: Signal delivery

### File I/O (fs.c)

- **open/close**: Flag translation (O_CREAT, O_RDWR, etc.)
- **read/write**: Chunked I/O with guest memory translation
- **stat/lstat/fstat**: Darwin→Linux stat structure conversion
- **Directory ops**: mkdir, rmdir, chdir, getcwd
- **Link ops**: link, unlink

### Memory Management (memory.c)

- **mmap**: Full translation with Darwin MAP_ANON support
- **munmap**: Memory unmapping
- **mprotect**: Protection flags translation
- **brk**: Heap management (64MB pre-allocated pool)

### Network (network.c)

- **socket**: Domain/type translation (AF_INET, SOCK_STREAM, etc.)
- **bind/connect/listen/accept**: Full socket lifecycle
- **send/recv**: Chunked network I/O

### User Access (user_access.c)

- **user_read/user_write**: Safe guest memory access
- **user_read_string/user_write_string**: String translation
- Memory bounds checking to prevent escapes

## Syscall Table

Linux ARM64 syscalls mapped to handlers:

| Number | Name   | Handler    | Notes                           |
| ------ | ------ | ---------- | ------------------------------- |
| 1      | exit   | sys_exit   | Process termination             |
| 2      | fork   | sys_fork   | via posix_spawn                 |
| 3      | read   | sys_read   | File/socket read                |
| 4      | write  | sys_write  | File/socket write               |
| 5      | open   | sys_open   | File open with flag translation |
| 6      | close  | sys_close  | File descriptor close           |
| 20     | getpid | sys_getpid | Process ID                      |
| 45     | brk    | sys_brk    | Heap management                 |
| 90/192 | mmap2  | sys_mmap2  | Memory mapping                  |
| 91     | munmap | sys_munmap | Memory unmapping                |
| 120    | clone  | sys_clone  | Thread creation                 |
| 195    | stat64 | sys_stat64 | File status                     |
| 281    | socket | sys_socket | Create socket                   |

## Integration with Emulator

Agent 2 (emulator) calls this when SVC instruction is executed:

```c
// In emulator when handling SVC instruction:
long result = handle_syscall(
    syscall_number,
    cpu->r0,  // arg1
    cpu->r1,  // arg2
    cpu->r2,  // arg3
    cpu->r3,  // arg4
    cpu->r4,  // arg5
    cpu->r5   // arg6
);
cpu->r0 = result;  // Return value
```

## Integration with Memory Manager

Set up guest memory for user_access functions:

```c
// Called by Agent 2 during initialization
extern void *emulator_memory_base;
extern size_t emulator_memory_size;

void init_syscalls(void *mem_base, size_t mem_size) {
    emulator_memory_base = mem_base;
    emulator_memory_size = mem_size;
}
```

## iOS 26 Constraints

### No Real Fork

- iOS doesn't support fork() for security reasons
- Implemented via posix_spawn with POSIX_SPAWN_SETEXEC
- Multi-process apps emulated through process table (future work)

### Sandbox Restrictions

- File access limited to app container
- Network restrictions based on entitlements
- Need to map Linux paths to iOS paths

### Memory Integrity

- W^X enforcement on iOS
- mmap with PROT_EXEC may fail
- JIT code requires special entitlements

## Error Handling

All syscalls return:

- **Positive values**: Success (often size, fd, or pid)
- **Negative values**: -errno (Linux error codes)

Example:

```c
dword_t result = sys_open(path_addr, O_RDONLY_, 0);
if (result < 0) {
    // Error: -result is errno (e.g., -ENOENT = -2)
} else {
    // Success: result is file descriptor
}
```

## Future Work

1. **Signal handling** (sys_sigaction, sys_sigprocmask)
2. **Advanced threading** (full clone() with CLONE_VM)
3. **Process table** for proper multi-process support
4. **Path translation** for iOS sandbox
5. **ioctl** translation for terminal operations
6. **More syscalls** (currently ~30 implemented, need ~200 for full Alpine)

## Testing

Compile test:

```bash
clang -c calls.c process.c fs.c memory.c network.c user_access.c \
      -I../include -Wall -Wextra -std=c11
```

## References

- **iSH Project**: Reference implementation (x86→Darwin)
- **Darling**: macOS syscall knowledge
- **Linux Syscall Table**: https://syscalls.w3challs.com/?arch=arm64
- **ARM64 Calling Convention**: Arguments in x0-x7
