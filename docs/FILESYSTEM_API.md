# Filesystem Layer API Quick Reference

**For Integration by Agents 1, 2, 3, and 4**

## Initialization

```c
#include "Filesystem/fakefs/fake.h"
#include "Filesystem/fakefs/proc_sys_dev.h"

struct fakefs global_fs;

// App startup - call once
int init_filesystem(const char *documents_path) {
    char db_path[1024], data_path[1024];
    snprintf(db_path, sizeof(db_path), "%s/rootfs/meta.db", documents_path);
    snprintf(data_path, sizeof(data_path), "%s/rootfs/data", documents_path);

    if (fakefs_init(&global_fs, db_path, data_path) < 0)
        return -1;

    // Mount virtual filesystems
    char path[1024];
    snprintf(path, sizeof(path), "%s/rootfs/data/proc", documents_path);
    procfs_init(path);
    snprintf(path, sizeof(path), "%s/rootfs/data/sys", documents_path);
    sysfs_init(path);
    snprintf(path, sizeof(path), "%s/rootfs/data/dev", documents_path);
    devfs_init(path);

    return 0;
}

// App shutdown - call once
void deinit_filesystem() {
    fakefs_deinit(&global_fs);
}
```

## File Operations

```c
// Open file
int fd = fakefs_open(&global_fs, "/bin/sh", O_RDONLY, 0);

// Read file
char buf[4096];
ssize_t n = fakefs_read(&global_fs, fd, buf, sizeof(buf));

// Write file
int fd = fakefs_open(&global_fs, "/tmp/test", O_WRONLY | O_CREAT, 0644);
fakefs_write(&global_fs, fd, "Hello\n", 6);

// Close file
fakefs_close(&global_fs, fd);
```

## File Metadata

```c
// Get file info
struct stat st;
fakefs_stat(&global_fs, "/etc/passwd", &st);
printf("Size: %lld, Mode: %o, Inode: %lld\n",
       st.st_size, st.st_mode, st.st_ino);

// Change permissions
fakefs_chmod(&global_fs, "/tmp/script.sh", 0755);

// Change owner
fakefs_chown(&global_fs, "/tmp/file", 1000, 1000);
```

## Directory Operations

```c
// Create directory
fakefs_mkdir(&global_fs, "/tmp/mydir", 0755);

// Remove directory
fakefs_rmdir(&global_fs, "/tmp/mydir");

// List directory
DIR *dirp = fakefs_opendir(&global_fs, "/bin");
struct dirent *ent;
while ((ent = fakefs_readdir(&global_fs, dirp)) != NULL) {
    printf("%s\n", ent->d_name);
}
fakefs_closedir(&global_fs, dirp);
```

## File Management

```c
// Delete file
fakefs_unlink(&global_fs, "/tmp/oldfile");

// Rename file
fakefs_rename(&global_fs, "/tmp/old", "/tmp/new");

// Create symlink
fakefs_symlink(&global_fs, "/bin/sh", "/bin/bash");

// Read symlink
char target[256];
ssize_t len = fakefs_readlink(&global_fs, "/bin/bash", target, sizeof(target));
target[len] = '\0';
```

## Syscall Mapping (Agent 4)

```c
// ARM64 Linux syscall numbers
#define SYS_openat       56
#define SYS_close        57
#define SYS_read         63
#define SYS_write        64
#define SYS_fstatat      79
#define SYS_mkdirat      34
#define SYS_unlinkat     35
#define SYS_renameat     38
#define SYS_symlinkat    36
#define SYS_readlinkat   78

// Dispatch table
long sys_openat(int dirfd, const char *pathname, int flags, mode_t mode) {
    // Ignore dirfd for now (AT_FDCWD)
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

long sys_fstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags) {
    if (flags & AT_SYMLINK_NOFOLLOW)
        return fakefs_lstat(&global_fs, pathname, statbuf);
    else
        return fakefs_stat(&global_fs, pathname, statbuf);
}
```

## Emulator Integration (Agent 2)

```c
void handle_syscall(struct cpu_state *cpu) {
    uint64_t syscall_num = cpu->regs[8];  // x8 on ARM64
    long ret;

    switch (syscall_num) {
        case SYS_openat:
            ret = sys_openat(
                cpu->regs[0],                    // dirfd
                (const char *)cpu->regs[1],      // pathname
                cpu->regs[2],                    // flags
                cpu->regs[3]                     // mode
            );
            break;

        case SYS_read:
            ret = sys_read(
                cpu->regs[0],                    // fd
                (void *)cpu->regs[1],            // buf
                cpu->regs[2]                     // count
            );
            break;

        case SYS_write:
            ret = sys_write(
                cpu->regs[0],                    // fd
                (const void *)cpu->regs[1],      // buf
                cpu->regs[2]                     // count
            );
            break;

        case SYS_close:
            ret = sys_close(cpu->regs[0]);       // fd
            break;

        case SYS_fstatat:
            ret = sys_fstatat(
                cpu->regs[0],                    // dirfd
                (const char *)cpu->regs[1],      // pathname
                (struct stat *)cpu->regs[2],     // statbuf
                cpu->regs[3]                     // flags
            );
            break;

        default:
            ret = -ENOSYS;
            break;
    }

    cpu->regs[0] = ret;  // Return value in x0
}
```

## Virtual Filesystems

### /proc

```c
// Automatically handled by fakefs layer
int fd = fakefs_open(&global_fs, "/proc/cpuinfo", O_RDONLY, 0);
char buf[1024];
fakefs_read(&global_fs, fd, buf, sizeof(buf));
// buf contains ARM64 CPU info
```

### /dev

```c
// /dev/null - discards writes, returns 0 on read
int fd = fakefs_open(&global_fs, "/dev/null", O_WRONLY, 0);
fakefs_write(&global_fs, fd, data, size);  // discarded

// /dev/zero - returns zeros
int fd = fakefs_open(&global_fs, "/dev/zero", O_RDONLY, 0);
fakefs_read(&global_fs, fd, buf, 100);  // buf filled with zeros

// /dev/urandom - random data
int fd = fakefs_open(&global_fs, "/dev/urandom", O_RDONLY, 0);
fakefs_read(&global_fs, fd, buf, 32);  // 32 random bytes
```

## Error Handling

All functions return standard POSIX error codes:

- `-1` on error, with `errno` set (if available)
- `>= 0` on success

Common errors:

- `ENOENT` - File not found
- `EEXIST` - File already exists
- `EISDIR` - Is a directory
- `ENOTDIR` - Not a directory
- `EACCES` - Permission denied
- `EINVAL` - Invalid argument

## Build Integration (Agent 1 - Xcode)

### Link Settings

```xml
<!-- In Xcode project -->
<OTHER_LDFLAGS>-lsqlite3</OTHER_LDFLAGS>
<HEADER_SEARCH_PATHS>$(PROJECT_DIR)</HEADER_SEARCH_PATHS>
```

### Add to Project

1. Drag `Filesystem/libfakefs.a` → Link Binary with Libraries
2. Drag `Alpine/rootfs/` → Copy Bundle Resources
3. Add `libsqlite3.tbd` system framework

### Runtime Paths

```swift
let docs = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
let rootfs = docs.appendingPathComponent("rootfs")

// Initialize C layer
init_filesystem(docs.path)
```

## Performance Notes

- First operation may be slow (SQLite warm-up)
- Subsequent operations are fast (prepared statements)
- WAL mode allows concurrent reads
- Keep transactions short for write operations
- Virtual filesystems (/proc, /dev) have no disk I/O

## Thread Safety

- ✅ **Thread-safe**: All operations use mutexes
- Use separate `struct fakefs` per thread for best performance
- Or share `global_fs` (automatically synchronized)

## Memory Usage

- Database: ~100 KB for Alpine rootfs
- Per-file overhead: ~40 bytes in SQLite
- Actual files stored on iOS filesystem
- Minimal RAM usage (no caching layer)

## Debugging

```c
// Enable verbose logging
#define DEBUG_FAKEFS 1

// Check database integrity
sqlite3 Alpine/rootfs/meta.db
sqlite> PRAGMA integrity_check;
OK

// View all paths
sqlite> SELECT path FROM paths LIMIT 10;
/bin
/bin/sh
/etc
/etc/passwd
...
```

---

**Need Help?**

- See `INTEGRATION.md` for detailed examples
- See `Filesystem/README.md` for implementation details
- See `FILESYSTEM_STATUS.md` for project status
