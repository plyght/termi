# termi Integration Guide

How all 4 parallel agents' work fits together.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     iOS 26 App (Xcode)                     │
│                       Agent 1: Xcode                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────┐          ┌──────────────────────┐    │
│  │  Terminal UI    │◄────────►│   ARM64 Emulator     │    │
│  │    Agent 3      │          │      Agent 2         │    │
│  │                 │          │                       │    │
│  │  - PTY handling │          │  - Loads binaries    │    │
│  │  - VT100/ANSI   │          │  - Executes ARM64    │    │
│  │  - Touch input  │          │  - JIT compilation   │    │
│  └────────┬────────┘          └──────────┬───────────┘    │
│           │                              │                 │
│           │                              │                 │
│           └──────────┬───────────────────┘                 │
│                      │                                     │
│                      ▼                                     │
│           ┌──────────────────────┐                        │
│           │   Syscall Layer      │                        │
│           │     Agent 4          │                        │
│           │                      │                        │
│           │  - Maps Linux calls  │                        │
│           │  - To iOS sandbox    │                        │
│           └──────────┬───────────┘                        │
│                      │                                     │
│                      ▼                                     │
│           ┌──────────────────────┐                        │
│           │  Filesystem Layer    │                        │
│           │   This Agent (You)   │                        │
│           │                      │                        │
│           │  - SQLite fake FS    │                        │
│           │  - Alpine rootfs     │                        │
│           │  - /proc /sys /dev   │                        │
│           └──────────────────────┘                        │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Filesystem Integration

### 1. Initialization (App Launch)

**File**: `AppDelegate.swift` or equivalent

```c
#include "Filesystem/fakefs/fake.h"
#include "Filesystem/fakefs/proc_sys_dev.h"

struct fakefs global_fs;

void app_init_filesystem() {
    NSString *docs = NSSearchPathForDirectoriesInDomains(
        NSDocumentDirectory, NSUserDomainMask, YES)[0];

    const char *docs_path = [docs UTF8String];
    char db_path[1024];
    char data_path[1024];

    snprintf(db_path, sizeof(db_path), "%s/rootfs/meta.db", docs_path);
    snprintf(data_path, sizeof(data_path), "%s/rootfs/data", docs_path);

    if (fakefs_init(&global_fs, db_path, data_path) < 0) {
        NSLog(@"Failed to initialize filesystem");
        exit(1);
    }

    char proc_path[1024], sys_path[1024], dev_path[1024];
    snprintf(proc_path, sizeof(proc_path), "%s/proc", data_path);
    snprintf(sys_path, sizeof(sys_path), "%s/sys", data_path);
    snprintf(dev_path, sizeof(dev_path), "%s/dev", data_path);

    procfs_init(proc_path);
    sysfs_init(sys_path);
    devfs_init(dev_path);

    NSLog(@"Filesystem initialized");
}
```

### 2. Syscall Layer Integration (Agent 4)

**File**: `Kernel/syscalls.c`

```c
#include "Filesystem/fakefs/fake.h"

extern struct fakefs global_fs;

long sys_open(const char *pathname, int flags, mode_t mode) {
    return fakefs_open(&global_fs, pathname, flags, mode);
}

long sys_read(int fd, void *buf, size_t count) {
    return fakefs_read(&global_fs, fd, buf, count);
}

long sys_write(int fd, const void *buf, size_t count) {
    return fakefs_write(&global_fs, fd, buf, count);
}

long sys_close(int fd) {
    return fakefs_close(&global_fs, fd);
}

long sys_stat(const char *pathname, struct stat *statbuf) {
    return fakefs_stat(&global_fs, pathname, statbuf);
}

long sys_mkdir(const char *pathname, mode_t mode) {
    return fakefs_mkdir(&global_fs, pathname, mode);
}

long sys_unlink(const char *pathname) {
    return fakefs_unlink(&global_fs, pathname);
}

long sys_rename(const char *oldpath, const char *newpath) {
    return fakefs_rename(&global_fs, oldpath, newpath);
}

long sys_symlink(const char *target, const char *linkpath) {
    return fakefs_symlink(&global_fs, target, linkpath);
}

long sys_readlink(const char *pathname, char *buf, size_t bufsiz) {
    return fakefs_readlink(&global_fs, pathname, buf, bufsiz);
}

long sys_chmod(const char *pathname, mode_t mode) {
    return fakefs_chmod(&global_fs, pathname, mode);
}

long sys_chown(const char *pathname, uid_t owner, gid_t group) {
    return fakefs_chown(&global_fs, pathname, owner, group);
}
```

### 3. Emulator Integration (Agent 2)

**File**: `Emulator/arm64_emulator.c`

```c
void handle_syscall(struct cpu_state *cpu) {
    uint64_t syscall_num = cpu->regs[8];  // ARM64 syscall convention

    switch (syscall_num) {
        case 56:  // SYS_openat
            cpu->regs[0] = sys_open(
                (const char *)cpu->regs[1],
                cpu->regs[2],
                cpu->regs[3]
            );
            break;

        case 63:  // SYS_read
            cpu->regs[0] = sys_read(
                cpu->regs[0],
                (void *)cpu->regs[1],
                cpu->regs[2]
            );
            break;

        case 64:  // SYS_write
            cpu->regs[0] = sys_write(
                cpu->regs[0],
                (const void *)cpu->regs[1],
                cpu->regs[2]
            );
            break;

        case 57:  // SYS_close
            cpu->regs[0] = sys_close(cpu->regs[0]);
            break;

        case 79:  // SYS_fstatat
            cpu->regs[0] = sys_stat(
                (const char *)cpu->regs[1],
                (struct stat *)cpu->regs[2]
            );
            break;

        case 34:  // SYS_mkdirat
            cpu->regs[0] = sys_mkdir(
                (const char *)cpu->regs[1],
                cpu->regs[2]
            );
            break;

        case 35:  // SYS_unlinkat
            cpu->regs[0] = sys_unlink(
                (const char *)cpu->regs[1]
            );
            break;

        default:
            cpu->regs[0] = -ENOSYS;
            break;
    }
}
```

### 4. Terminal UI Integration (Agent 3)

**File**: `UI/TerminalViewController.swift`

The Terminal UI primarily interacts with the emulator via PTY, but can directly access virtual filesystems for debugging:

```swift
func debugFilesystem() {
    let fm = FileManager.default
    let docs = fm.urls(for: .documentDirectory, in: .userDomainMask)[0]

    // Read /proc/cpuinfo
    let cpuinfoPath = docs.appendingPathComponent("rootfs/data/proc/cpuinfo")
    if let cpuinfo = try? String(contentsOf: cpuinfoPath) {
        print("CPU Info:\n\(cpuinfo)")
    }

    // List rootfs
    let rootfsPath = docs.appendingPathComponent("rootfs/data")
    if let contents = try? fm.contentsOfDirectory(atPath: rootfsPath.path) {
        print("Rootfs contents: \(contents)")
    }
}
```

## Build Process

### 1. Build Filesystem Layer

```bash
cd Filesystem
make
```

Produces:

- `libfakefs.a` - Static library for iOS app
- `fakefsify` - CLI tool for rootfs management

### 2. Import Alpine Rootfs

```bash
cd Alpine
chmod +x setup_alpine.sh
./setup_alpine.sh
```

Produces:

- `Alpine/rootfs/data/` - Filesystem contents
- `Alpine/rootfs/meta.db` - SQLite metadata

### 3. Xcode Integration

Add to Xcode project:

1. Drag `libfakefs.a` into Link Binary with Libraries
2. Add `Filesystem/` to Header Search Paths
3. Link against `libsqlite3.tbd`
4. Copy `Alpine/rootfs/` to app bundle resources

**Build Settings**:

```
OTHER_LDFLAGS = -lsqlite3
HEADER_SEARCH_PATHS = $(PROJECT_DIR)/Filesystem
```

## Data Flow Example: `ls /bin`

1. **Terminal UI** sends command via PTY
2. **Emulator** executes `/bin/ls` ARM64 binary
3. `/bin/ls` makes `opendir("/bin")` syscall
4. **Syscall Layer** translates to `sys_opendir`
5. **Filesystem** queries SQLite: `SELECT * FROM paths WHERE path LIKE '/bin%'`
6. Returns directory entries
7. **Emulator** receives file list
8. `/bin/ls` formats output
9. **Terminal UI** displays colorized output

## Virtual Filesystem Behavior

### /proc

- Emulator calls `procfs_read("/proc/cpuinfo", buf, size)`
- Returns ARM64 CPU info
- No actual file exists

### /dev/null

- Writes discarded: `devfs_write("/dev/null", data, len)` → len
- Reads return nothing: `devfs_read("/dev/null", buf, size)` → 0

### /dev/urandom

- Returns random bytes: `devfs_read("/dev/urandom", buf, size)` → random data

## Performance Considerations

1. **SQLite Indexing**: All path queries use index
2. **WAL Mode**: Concurrent reads don't block
3. **Prepared Statements**: Pre-compiled for hot paths
4. **Metadata Caching**: Consider LRU cache for frequently accessed inodes

## Debugging

Enable verbose logging:

```c
#define DEBUG_FAKEFS 1

// In fake.c:
if (DEBUG_FAKEFS) {
    fprintf(stderr, "fakefs_open(%s, %d, %o)\n", path, flags, mode);
}
```

Check SQLite integrity:

```bash
sqlite3 Alpine/rootfs/meta.db
sqlite> PRAGMA integrity_check;
sqlite> SELECT COUNT(*) FROM paths;
sqlite> SELECT path FROM paths LIMIT 10;
```

## Next Steps

1. **Agent 2** (Emulator): Implement syscall dispatch table
2. **Agent 3** (Terminal UI): Handle PTY I/O
3. **Agent 4** (Syscall Layer): Complete syscall coverage
4. **This Agent**: Optimize performance, add caching

All agents can proceed in parallel using these interfaces.
