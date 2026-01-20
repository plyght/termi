# Syscall Translation Layer - Integration Guide

## For Agent 2 (ARM64 Emulator Team)

### 1. Include Headers

```c
#include "Kernel/syscall/calls.h"
#include "Kernel/include/types.h"
```

### 2. Initialize Memory Manager

Before handling any syscalls, set up guest memory access:

```c
void *guest_memory = /* your emulated RAM base */;
size_t memory_size = /* your RAM size, e.g., 256MB */;

extern void *emulator_memory_base;
extern size_t emulator_memory_size;

void init_emulator_syscalls(void) {
    emulator_memory_base = guest_memory;
    emulator_memory_size = memory_size;
}
```

### 3. Handle SVC Instructions

When your emulator encounters an `SVC #0` instruction (ARM64 syscall):

```c
void handle_svc_instruction(struct cpu_state *cpu) {
    int syscall_num = cpu->x8;  // Linux ARM64 syscall number in x8

    long result = handle_syscall(
        syscall_num,
        cpu->x0,  // arg1
        cpu->x1,  // arg2
        cpu->x2,  // arg3
        cpu->x3,  // arg4
        cpu->x4,  // arg5
        cpu->x5   // arg6
    );

    cpu->x0 = result;  // Return value in x0

    // Handle errors (negative return values are -errno)
    if (result < 0) {
        // Optionally set up errno for guest
        // Most programs check return value directly
    }
}
```

### 4. Link Against Library

Add to your build:

```makefile
SYSCALL_LIB = Kernel/syscall/libsyscall.a
CFLAGS += -IKernel/include -IKernel/syscall

emulator: emulator.o cpu.o $(SYSCALL_LIB)
	$(CC) -o $@ $^ -framework Foundation
```

### 5. Memory Layout Considerations

The syscall layer expects guest memory at fixed addresses:

```
Guest Address Space:
0x00000000 - 0x00400000    : Text/Code (4MB)
0x00400000 - 0x08000000    : Data/Heap (124MB)
0x08000000 - 0x10000000    : Stack (128MB, grows down)
0x10000000+                : mmap region

Your emulator must map these guest addresses to host memory.
```

## For Agent 5 (Filesystem Team)

### Path Translation Hook

The syscall layer will need path translation for iOS sandbox:

```c
// Add this to fs.c integration
extern char *translate_guest_path(const char *linux_path);

// Example usage:
dword_t sys_open(addr_t path_addr, dword_t flags, mode_t_ mode) {
    char linux_path[4096];
    user_read_string(path_addr, linux_path, sizeof(linux_path));

    char *ios_path = translate_guest_path(linux_path);
    int fd = open(ios_path, darwin_flags, mode);
    free(ios_path);
    // ...
}
```

### Required Path Mappings

```
Linux Path              → iOS Path
/                       → <app>/alpine_root/
/tmp                    → <app>/tmp/
/home/user              → <app>/Documents/
/dev/null               → /dev/null (pass through)
/dev/urandom            → /dev/urandom (pass through)
/proc/*                 → Virtual filesystem (future)
```

## For Agent 3 (Terminal UI Team)

### Terminal I/O Integration

File descriptors 0, 1, 2 (stdin, stdout, stderr) should connect to your terminal:

```c
// In your terminal initialization
int pty_master = /* your PTY master fd */;

// In emulator setup
dup2(pty_master, STDIN_FILENO);   // fd 0
dup2(pty_master, STDOUT_FILENO);  // fd 1
dup2(pty_master, STDERR_FILENO);  // fd 2

// Now sys_read(0, ...) and sys_write(1, ...) go to your terminal
```

### ioctl Support (Future)

Terminal operations need ioctl translation:

```c
// Future work - add to fs.c
dword_t sys_ioctl(fd_t fd, dword_t cmd, addr_t arg) {
    switch (cmd) {
        case TCGETS_:  // Get terminal attributes
            // Translate to Darwin TIOCGETA
        case TCSETS_:  // Set terminal attributes
            // Translate to Darwin TIOCSETA
        case TIOCGWINSZ_:  // Get window size
            // Translate to Darwin TIOCGWINSZ
    }
}
```

## Error Handling

All syscalls return:

- **≥ 0**: Success (value depends on syscall)
- **< 0**: Error (value is `-errno`)

Example handling in emulator:

```c
long result = handle_syscall(num, arg1, arg2, arg3, arg4, arg5, arg6);

if (result < 0) {
    int linux_errno = -result;

    switch (linux_errno) {
        case _ENOENT:  fprintf(stderr, "File not found\n"); break;
        case _EACCES:  fprintf(stderr, "Permission denied\n"); break;
        case _ENOMEM:  fprintf(stderr, "Out of memory\n"); break;
        default:       fprintf(stderr, "Error %d\n", linux_errno); break;
    }
}
```

## Current Syscall Support

Implemented (33 syscalls):

| Category   | Syscalls                                                         |
| ---------- | ---------------------------------------------------------------- |
| Process    | exit, fork, execve, getpid, getppid, wait4, waitpid, clone, kill |
| File I/O   | open, close, read, write, stat64, lstat64, fstat64               |
| Filesystem | chdir, getcwd, mkdir, rmdir, link, unlink                        |
| Memory     | brk, mmap2, munmap, mprotect                                     |
| Network    | socket, bind, connect, listen, accept, send, recv                |
| Helpers    | user_read, user_write, user_read_string, user_write_string       |

## Testing the Integration

### Test 1: Basic Syscall

```c
// Create a test that calls sys_getpid
int main() {
    long pid = handle_syscall(20, 0, 0, 0, 0, 0, 0);  // syscall 20 = getpid
    printf("PID: %ld\n", pid);
    return 0;
}
```

### Test 2: File I/O

```c
// Test file operations
void test_file_io() {
    // Assume guest memory is set up
    char *path = "/tmp/test.txt";
    write_string_to_guest(0x1000, path);

    // Open file
    long fd = handle_syscall(5, 0x1000, O_CREAT_|O_WRONLY_, 0644, 0, 0, 0);
    assert(fd >= 0);

    // Write data
    char *data = "Hello, termi!";
    write_string_to_guest(0x2000, data);
    long written = handle_syscall(4, fd, 0x2000, 13, 0, 0, 0);
    assert(written == 13);

    // Close
    long result = handle_syscall(6, fd, 0, 0, 0, 0, 0);
    assert(result == 0);
}
```

## Performance Considerations

1. **User Memory Access**: Chunked I/O reduces overhead
2. **Syscall Dispatch**: Table lookup is O(1)
3. **String Operations**: Bounded to prevent infinite loops
4. **Memory Bounds**: All accesses validated

## Future Enhancements

1. **Signal Handling**: sigaction, sigprocmask, kill improvements
2. **Advanced Clone**: CLONE_THREAD for real threading
3. **ioctl**: Terminal and device control
4. **Virtual Filesystems**: /proc, /sys emulation
5. **Futex**: For pthread synchronization
6. **Advanced Networking**: sendmsg, recvmsg, socket options

## Debugging

Enable syscall tracing:

```c
// In calls.c, modify handle_syscall:
long handle_syscall(int num, long arg1, long arg2, long arg3,
                   long arg4, long arg5, long arg6) {
    #ifdef SYSCALL_TRACE
    fprintf(stderr, "SYSCALL %d(%ld, %ld, %ld, %ld, %ld, %ld)\n",
            num, arg1, arg2, arg3, arg4, arg5, arg6);
    #endif

    // ... rest of function

    #ifdef SYSCALL_TRACE
    fprintf(stderr, "  -> %ld\n", result);
    #endif
    return result;
}
```

Compile with: `CFLAGS += -DSYSCALL_TRACE`

## Questions?

Contact the kernel team or check:

- `Kernel/syscall/README.md` - Architecture details
- `Kernel/syscall/calls.h` - Syscall declarations
- iSH project source - Reference implementation
