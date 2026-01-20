#include "calls.h"
#include "../../Filesystem/fakefs/fake.h"
#include "../../Filesystem/fakefs/proc_sys_dev.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern struct fakefs global_fakefs;

#define O_RDONLY_ 0x0000
#define O_WRONLY_ 0x0001
#define O_RDWR_ 0x0002
#define O_CREAT_ 0x0040
#define O_EXCL_ 0x0080
#define O_TRUNC_ 0x0200
#define O_APPEND_ 0x0400
#define O_NONBLOCK_ 0x0800
#define O_DIRECTORY_ 0x010000
#define O_CLOEXEC_ 0x080000

static int translate_open_flags(dword_t linux_flags)
{
    int darwin_flags = 0;

    switch (linux_flags & 0x3) {
    case O_RDONLY_:
        darwin_flags |= O_RDONLY;
        break;
    case O_WRONLY_:
        darwin_flags |= O_WRONLY;
        break;
    case O_RDWR_:
        darwin_flags |= O_RDWR;
        break;
    }

    if (linux_flags & O_CREAT_)
        darwin_flags |= O_CREAT;
    if (linux_flags & O_EXCL_)
        darwin_flags |= O_EXCL;
    if (linux_flags & O_TRUNC_)
        darwin_flags |= O_TRUNC;
    if (linux_flags & O_APPEND_)
        darwin_flags |= O_APPEND;
    if (linux_flags & O_NONBLOCK_)
        darwin_flags |= O_NONBLOCK;
    if (linux_flags & O_DIRECTORY_)
        darwin_flags |= O_DIRECTORY;
    if (linux_flags & O_CLOEXEC_)
        darwin_flags |= O_CLOEXEC;

    return darwin_flags;
}

dword_t sys_open(addr_t path_addr, dword_t flags, mode_t_ mode)
{
    char path[4096];

    if (user_read_string(path_addr, path, sizeof(path)) < 0) {
        return _EFAULT;
    }

    printf("[syscall] sys_open: path=%s, flags=0x%x, mode=0%o\n", path, flags, mode);

    int darwin_flags = translate_open_flags(flags);
    int fd = fakefs_open(&global_fakefs, path, darwin_flags, mode);

    if (fd < 0) {
        if (strncmp(path, "/dev/", 5) == 0 || strncmp(path, "/proc/", 6) == 0) {
            printf("[syscall] sys_open: fakefs failed for %s, trying Darwin fallback\n", path);
            fd = open(path, darwin_flags, mode);
            if (fd < 0) {
                return -errno;
            }
        } else {
            return -errno;
        }
    }

    printf("[syscall] sys_open: fd=%d\n", fd);
    return fd;
}

dword_t sys_close(fd_t fd)
{
    printf("[syscall] sys_close: fd=%d\n", fd);
    
    int result = fakefs_close(&global_fakefs, fd);
    if (result < 0) {
        printf("[syscall] sys_close: fakefs failed, trying Darwin close\n");
        if (close(fd) < 0) {
            return -errno;
        }
        return 0;
    }
    return 0;
}

dword_t sys_read(fd_t fd_no, addr_t buf_addr, dword_t size)
{
    char buffer[8192];
    dword_t total_read = 0;

    while (size > 0) {
        size_t chunk = (size_t)size > sizeof(buffer) ? sizeof(buffer) : (size_t)size;
        ssize_t nread = fakefs_read(&global_fakefs, fd_no, buffer, chunk);

        if (nread < 0) {
            printf("[syscall] sys_read: fakefs_read failed for fd=%d, trying Darwin read\n", fd_no);
            nread = read(fd_no, buffer, chunk);
            if (nread < 0) {
                return total_read > 0 ? total_read : -errno;
            }
        }

        if (nread == 0) {
            break;
        }

        if (user_write(buf_addr + total_read, buffer, nread) < 0) {
            return total_read > 0 ? total_read : _EFAULT;
        }

        total_read += nread;
        size -= nread;

        if (nread < (ssize_t)chunk) {
            break;
        }
    }

    printf("[syscall] sys_read: fd=%d, total_read=%d\n", fd_no, total_read);
    return total_read;
}

static char stdout_buffer[16384];
static size_t stdout_buffer_pos = 0;
static char stderr_buffer[16384];
static size_t stderr_buffer_pos = 0;

dword_t sys_write(fd_t fd_no, addr_t buf_addr, dword_t size)
{
    char buffer[8192];
    dword_t total_written = 0;

    while (size > 0) {
        size_t chunk = (size_t)size > sizeof(buffer) ? sizeof(buffer) : (size_t)size;

        if (user_read(buf_addr + total_written, buffer, chunk) < 0) {
            return total_written > 0 ? total_written : _EFAULT;
        }

        if (fd_no == 1 || fd_no == 2) {
            char *term_buffer = (fd_no == 1) ? stdout_buffer : stderr_buffer;
            size_t *term_pos = (fd_no == 1) ? &stdout_buffer_pos : &stderr_buffer_pos;
            size_t term_size = sizeof(stdout_buffer);

            size_t copy_size = chunk;
            if (*term_pos + copy_size > term_size - 1) {
                copy_size = term_size - 1 - *term_pos;
            }
            if (copy_size > 0) {
                memcpy(term_buffer + *term_pos, buffer, copy_size);
                *term_pos += copy_size;
                term_buffer[*term_pos] = '\0';
            }
        }

        ssize_t nwritten = fakefs_write(&global_fakefs, fd_no, buffer, chunk);

        if (nwritten < 0) {
            printf("[syscall] sys_write: fakefs_write failed for fd=%d, trying Darwin write\n", fd_no);
            nwritten = write(fd_no, buffer, chunk);
            if (nwritten < 0) {
                return total_written > 0 ? total_written : -errno;
            }
        }

        total_written += nwritten;
        size -= nwritten;

        if (nwritten < (ssize_t)chunk) {
            break;
        }
    }

    printf("[syscall] sys_write: fd=%d, total_written=%d\n", fd_no, total_written);
    return total_written;
}

#include <stdint.h>

struct linux_stat64 {
    uint64_t st_dev;
    uint64_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    uint64_t st_size;
    uint32_t st_blksize;
    uint64_t st_blocks;
    uint32_t st_atime_sec;
    uint32_t st_atime_nsec;
    uint32_t st_mtime_sec;
    uint32_t st_mtime_nsec;
    uint32_t st_ctime_sec;
    uint32_t st_ctime_nsec;
};

static void darwin_to_linux_stat(struct stat *darwin_stat, struct linux_stat64 *linux_stat)
{
    memset(linux_stat, 0, sizeof(*linux_stat));
    linux_stat->st_dev = darwin_stat->st_dev;
    linux_stat->st_ino = darwin_stat->st_ino;
    linux_stat->st_mode = darwin_stat->st_mode;
    linux_stat->st_nlink = darwin_stat->st_nlink;
    linux_stat->st_uid = darwin_stat->st_uid;
    linux_stat->st_gid = darwin_stat->st_gid;
    linux_stat->st_rdev = darwin_stat->st_rdev;
    linux_stat->st_size = darwin_stat->st_size;
    linux_stat->st_blksize = darwin_stat->st_blksize;
    linux_stat->st_blocks = darwin_stat->st_blocks;
    linux_stat->st_atime_sec = darwin_stat->st_atimespec.tv_sec;
    linux_stat->st_atime_nsec = darwin_stat->st_atimespec.tv_nsec;
    linux_stat->st_mtime_sec = darwin_stat->st_mtimespec.tv_sec;
    linux_stat->st_mtime_nsec = darwin_stat->st_mtimespec.tv_nsec;
    linux_stat->st_ctime_sec = darwin_stat->st_ctimespec.tv_sec;
    linux_stat->st_ctime_nsec = darwin_stat->st_ctimespec.tv_nsec;
}

dword_t sys_stat64(addr_t path_addr, addr_t statbuf_addr)
{
    char path[4096];
    struct stat st;
    struct linux_stat64 lst;

    if (user_read_string(path_addr, path, sizeof(path)) < 0) {
        return _EFAULT;
    }

    printf("[syscall] sys_stat64: path=%s\n", path);

    int result = fakefs_stat(&global_fakefs, path, &st);
    if (result < 0) {
        if (strncmp(path, "/dev/", 5) == 0 || strncmp(path, "/proc/", 6) == 0) {
            printf("[syscall] sys_stat64: fakefs failed for %s, trying Darwin stat\n", path);
            if (stat(path, &st) < 0) {
                return -errno;
            }
        } else {
            return -errno;
        }
    }

    darwin_to_linux_stat(&st, &lst);

    if (user_write(statbuf_addr, &lst, sizeof(lst)) < 0) {
        return _EFAULT;
    }

    return 0;
}

dword_t sys_lstat64(addr_t path_addr, addr_t statbuf_addr)
{
    char path[4096];
    struct stat st;
    struct linux_stat64 lst;

    if (user_read_string(path_addr, path, sizeof(path)) < 0) {
        return _EFAULT;
    }

    printf("[syscall] sys_lstat64: path=%s\n", path);

    int result = fakefs_lstat(&global_fakefs, path, &st);
    if (result < 0) {
        if (strncmp(path, "/dev/", 5) == 0 || strncmp(path, "/proc/", 6) == 0) {
            printf("[syscall] sys_lstat64: fakefs failed for %s, trying Darwin lstat\n", path);
            if (lstat(path, &st) < 0) {
                return -errno;
            }
        } else {
            return -errno;
        }
    }

    darwin_to_linux_stat(&st, &lst);

    if (user_write(statbuf_addr, &lst, sizeof(lst)) < 0) {
        return _EFAULT;
    }

    return 0;
}

dword_t sys_fstat64(fd_t fd_no, addr_t statbuf_addr)
{
    struct stat st;
    struct linux_stat64 lst;

    printf("[syscall] sys_fstat64: fd=%d\n", fd_no);

    int result = fakefs_fstat(&global_fakefs, fd_no, &st);
    if (result < 0) {
        printf("[syscall] sys_fstat64: fakefs failed for fd=%d, trying Darwin fstat\n", fd_no);
        if (fstat(fd_no, &st) < 0) {
            return -errno;
        }
    }

    darwin_to_linux_stat(&st, &lst);

    if (user_write(statbuf_addr, &lst, sizeof(lst)) < 0) {
        return _EFAULT;
    }

    return 0;
}

static char virtual_cwd[4096] = "/";

dword_t sys_getcwd(addr_t buf_addr, dword_t size)
{
    printf("[syscall] sys_getcwd\n");

    size_t len = strlen(virtual_cwd) + 1;
    if (len > (size_t)size) {
        return _ERANGE;
    }

    if (user_write_string(buf_addr, virtual_cwd) < 0) {
        return _EFAULT;
    }

    printf("[syscall] sys_getcwd: returning %s\n", virtual_cwd);
    return len;
}

dword_t sys_chdir(addr_t path_addr)
{
    char path[4096];
    struct stat st;

    if (user_read_string(path_addr, path, sizeof(path)) < 0) {
        return _EFAULT;
    }

    printf("[syscall] sys_chdir: path=%s\n", path);

    int result = fakefs_stat(&global_fakefs, path, &st);
    if (result < 0) {
        return -errno;
    }

    if (!S_ISDIR(st.st_mode)) {
        return -_ENOTDIR;
    }

    if (path[0] == '/') {
        strncpy(virtual_cwd, path, sizeof(virtual_cwd) - 1);
        virtual_cwd[sizeof(virtual_cwd) - 1] = '\0';
    } else {
        size_t cwd_len = strlen(virtual_cwd);
        if (cwd_len > 0 && virtual_cwd[cwd_len - 1] != '/') {
            strncat(virtual_cwd, "/", sizeof(virtual_cwd) - cwd_len - 1);
        }
        strncat(virtual_cwd, path, sizeof(virtual_cwd) - strlen(virtual_cwd) - 1);
    }

    printf("[syscall] sys_chdir: new cwd=%s\n", virtual_cwd);
    return 0;
}

dword_t sys_mkdir(addr_t path_addr, mode_t_ mode)
{
    char path[4096];

    if (user_read_string(path_addr, path, sizeof(path)) < 0) {
        return _EFAULT;
    }

    printf("[syscall] sys_mkdir: path=%s, mode=0%o\n", path, mode);

    int result = fakefs_mkdir(&global_fakefs, path, mode);
    if (result < 0) {
        return -errno;
    }

    return 0;
}

dword_t sys_rmdir(addr_t path_addr)
{
    char path[4096];

    if (user_read_string(path_addr, path, sizeof(path)) < 0) {
        return _EFAULT;
    }

    printf("[syscall] sys_rmdir: path=%s\n", path);

    int result = fakefs_rmdir(&global_fakefs, path);
    if (result < 0) {
        return -errno;
    }

    return 0;
}

dword_t sys_link(addr_t src_addr, addr_t dst_addr)
{
    char src[4096], dst[4096];

    if (user_read_string(src_addr, src, sizeof(src)) < 0 ||
        user_read_string(dst_addr, dst, sizeof(dst)) < 0) {
        return _EFAULT;
    }

    printf("[syscall] sys_link: src=%s, dst=%s (using Darwin fallback)\n", src, dst);

    if (link(src, dst) < 0) {
        return -errno;
    }

    return 0;
}

dword_t sys_unlink(addr_t path_addr)
{
    char path[4096];

    if (user_read_string(path_addr, path, sizeof(path)) < 0) {
        return _EFAULT;
    }

    printf("[syscall] sys_unlink: path=%s\n", path);

    int result = fakefs_unlink(&global_fakefs, path);
    if (result < 0) {
        return -errno;
    }

    return 0;
}

const char *get_stdout_buffer(void)
{
    return stdout_buffer;
}

const char *get_stderr_buffer(void)
{
    return stderr_buffer;
}

void clear_stdout_buffer(void)
{
    stdout_buffer_pos = 0;
    stdout_buffer[0] = '\0';
}

void clear_stderr_buffer(void)
{
    stderr_buffer_pos = 0;
    stderr_buffer[0] = '\0';
}
