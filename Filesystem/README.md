# termi Filesystem Layer

SQLite-based virtual filesystem for Alpine Linux ARM64 on iOS 26.

## Architecture

Based on iSH's fake filesystem design:

- **fake-db.c/h**: SQLite metadata layer storing inodes, paths, and stat info
- **fake.c/h**: POSIX filesystem operations interface
- **proc_sys_dev.c/h**: Virtual `/proc`, `/sys`, `/dev` emulation
- **fakefsify**: Tool to import/export tar.gz archives to fake filesystem

## Building

```bash
cd Filesystem
make
```

Requires:

- `sqlite3` development libraries
- `libarchive` development libraries

## Usage

### Import Alpine Linux ARM64 rootfs

```bash
# Download Alpine minirootfs
curl -LO https://dl-cdn.alpinelinux.org/alpine/v3.19/releases/aarch64/alpine-minirootfs-3.19.0-aarch64.tar.gz

# Import to fake filesystem
./fakefsify alpine-minirootfs-3.19.0-aarch64.tar.gz ../Alpine/rootfs

# This creates:
# Alpine/rootfs/data/        - Actual file contents
# Alpine/rootfs/meta.db      - SQLite database with metadata
```

### Export filesystem

```bash
./unfakefsify ../Alpine/rootfs output.tar.gz
```

## Database Schema

```sql
CREATE TABLE meta (
    id INTEGER UNIQUE DEFAULT 0,
    db_inode INTEGER
);

CREATE TABLE stats (
    inode INTEGER PRIMARY KEY,
    stat BLOB  -- Packed struct: mode, uid, gid, rdev
);

CREATE TABLE paths (
    path BLOB PRIMARY KEY,
    inode INTEGER REFERENCES stats(inode)
);

CREATE INDEX inode_to_path ON paths (inode, path);
```

## Integration with Other Layers

### Emulator (Agent 2)

The ARM64 emulator calls filesystem operations via syscall layer:

```c
int sys_open(const char *path, int flags, mode_t mode) {
    return fakefs_open(&global_fs, path, flags, mode);
}
```

### Syscall Layer (Agent 4)

Maps Linux syscalls to filesystem operations:

- `SYS_open` → `fakefs_open`
- `SYS_read` → `fakefs_read`
- `SYS_stat` → `fakefs_stat`
- etc.

### Terminal UI (Agent 3)

Can mount virtual filesystems for display:

```c
procfs_init("/proc");
devfs_init("/dev");
sysfs_init("/sys");
```

## Performance Optimizations

1. **WAL Mode**: Write-Ahead Logging for concurrent reads
2. **Prepared Statements**: All SQL queries pre-compiled
3. **Transaction Batching**: Bulk operations use single transaction
4. **Mutex Locking**: Fast mutexes for thread safety

## Virtual Filesystems

### /proc

- `/proc/cpuinfo` - CPU information (ARM64)
- `/proc/meminfo` - Memory statistics
- `/proc/version` - Kernel version
- `/proc/uptime` - System uptime

### /sys

- `/sys/devices/` - Device tree
- `/sys/class/` - Device classes

### /dev

- `/dev/null` - Null device
- `/dev/zero` - Zero device
- `/dev/random` - Random number generator
- `/dev/urandom` - Pseudo-random generator
- `/dev/pts/` - Pseudo-terminal slaves

## iOS Integration

All data stored in app Documents directory:

```
Documents/
  ├── rootfs/
  │   ├── data/          # Actual files
  │   └── meta.db        # Metadata database
  ├── proc/              # Virtual /proc
  ├── sys/               # Virtual /sys
  └── dev/               # Virtual /dev
```

## Case Sensitivity

Uses APFS case-sensitive properly. Path normalization prevents:

- Path traversal attacks (`..` components)
- Case conflicts on iOS filesystem

## File Operations

All standard POSIX operations supported:

- `open`, `read`, `write`, `close`
- `stat`, `lstat`, `fstat`
- `mkdir`, `rmdir`, `unlink`, `rename`
- `symlink`, `readlink`
- `chmod`, `chown`
- `opendir`, `readdir`, `closedir`

## License

Same as termi project (see root LICENSE).
