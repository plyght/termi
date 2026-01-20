#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include "calls.h"

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

static int translate_open_flags(dword_t linux_flags) {
    int darwin_flags = 0;
    
    switch (linux_flags & 0x3) {
        case O_RDONLY_: darwin_flags |= O_RDONLY; break;
        case O_WRONLY_: darwin_flags |= O_WRONLY; break;
        case O_RDWR_: darwin_flags |= O_RDWR; break;
    }
    
    if (linux_flags & O_CREAT_) darwin_flags |= O_CREAT;
    if (linux_flags & O_EXCL_) darwin_flags |= O_EXCL;
    if (linux_flags & O_TRUNC_) darwin_flags |= O_TRUNC;
    if (linux_flags & O_APPEND_) darwin_flags |= O_APPEND;
    if (linux_flags & O_NONBLOCK_) darwin_flags |= O_NONBLOCK;
    if (linux_flags & O_DIRECTORY_) darwin_flags |= O_DIRECTORY;
    if (linux_flags & O_CLOEXEC_) darwin_flags |= O_CLOEXEC;
    
    return darwin_flags;
}

dword_t sys_open(addr_t path_addr, dword_t flags, mode_t_ mode) {
    char path[4096];
    
    if (user_read_string(path_addr, path, sizeof(path)) < 0) {
        return _EFAULT;
    }
    
    int darwin_flags = translate_open_flags(flags);
    int fd = open(path, darwin_flags, mode);
    
    if (fd < 0) {
        return -errno;
    }
    
    return fd;
}

dword_t sys_close(fd_t fd) {
    if (close(fd) < 0) {
        return -errno;
    }
    return 0;
}

dword_t sys_read(fd_t fd_no, addr_t buf_addr, dword_t size) {
    char buffer[8192];
    dword_t total_read = 0;
    
    while (size > 0) {
        size_t chunk = (size_t)size > sizeof(buffer) ? sizeof(buffer) : (size_t)size;
        ssize_t nread = read(fd_no, buffer, chunk);
        
        if (nread < 0) {
            return total_read > 0 ? total_read : -errno;
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
    
    return total_read;
}

dword_t sys_write(fd_t fd_no, addr_t buf_addr, dword_t size) {
    char buffer[8192];
    dword_t total_written = 0;
    
    while (size > 0) {
        size_t chunk = (size_t)size > sizeof(buffer) ? sizeof(buffer) : (size_t)size;
        
        if (user_read(buf_addr + total_written, buffer, chunk) < 0) {
            return total_written > 0 ? total_written : _EFAULT;
        }
        
        ssize_t nwritten = write(fd_no, buffer, chunk);
        
        if (nwritten < 0) {
            return total_written > 0 ? total_written : -errno;
        }
        
        total_written += nwritten;
        size -= nwritten;
        
        if (nwritten < (ssize_t)chunk) {
            break;
        }
    }
    
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

static void darwin_to_linux_stat(struct stat *darwin_stat, struct linux_stat64 *linux_stat) {
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

dword_t sys_stat64(addr_t path_addr, addr_t statbuf_addr) {
    char path[4096];
    struct stat st;
    struct linux_stat64 lst;
    
    if (user_read_string(path_addr, path, sizeof(path)) < 0) {
        return _EFAULT;
    }
    
    if (stat(path, &st) < 0) {
        return -errno;
    }
    
    darwin_to_linux_stat(&st, &lst);
    
    if (user_write(statbuf_addr, &lst, sizeof(lst)) < 0) {
        return _EFAULT;
    }
    
    return 0;
}

dword_t sys_lstat64(addr_t path_addr, addr_t statbuf_addr) {
    char path[4096];
    struct stat st;
    struct linux_stat64 lst;
    
    if (user_read_string(path_addr, path, sizeof(path)) < 0) {
        return _EFAULT;
    }
    
    if (lstat(path, &st) < 0) {
        return -errno;
    }
    
    darwin_to_linux_stat(&st, &lst);
    
    if (user_write(statbuf_addr, &lst, sizeof(lst)) < 0) {
        return _EFAULT;
    }
    
    return 0;
}

dword_t sys_fstat64(fd_t fd_no, addr_t statbuf_addr) {
    struct stat st;
    struct linux_stat64 lst;
    
    if (fstat(fd_no, &st) < 0) {
        return -errno;
    }
    
    darwin_to_linux_stat(&st, &lst);
    
    if (user_write(statbuf_addr, &lst, sizeof(lst)) < 0) {
        return _EFAULT;
    }
    
    return 0;
}

dword_t sys_getcwd(addr_t buf_addr, dword_t size) {
    char cwd[4096];
    
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return -errno;
    }
    
    size_t len = strlen(cwd) + 1;
    if (len > (size_t)size) {
        return _ERANGE;
    }
    
    if (user_write_string(buf_addr, cwd) < 0) {
        return _EFAULT;
    }
    
    return len;
}

dword_t sys_chdir(addr_t path_addr) {
    char path[4096];
    
    if (user_read_string(path_addr, path, sizeof(path)) < 0) {
        return _EFAULT;
    }
    
    if (chdir(path) < 0) {
        return -errno;
    }
    
    return 0;
}

dword_t sys_mkdir(addr_t path_addr, mode_t_ mode) {
    char path[4096];
    
    if (user_read_string(path_addr, path, sizeof(path)) < 0) {
        return _EFAULT;
    }
    
    if (mkdir(path, mode) < 0) {
        return -errno;
    }
    
    return 0;
}

dword_t sys_rmdir(addr_t path_addr) {
    char path[4096];
    
    if (user_read_string(path_addr, path, sizeof(path)) < 0) {
        return _EFAULT;
    }
    
    if (rmdir(path) < 0) {
        return -errno;
    }
    
    return 0;
}

dword_t sys_link(addr_t src_addr, addr_t dst_addr) {
    char src[4096], dst[4096];
    
    if (user_read_string(src_addr, src, sizeof(src)) < 0 ||
        user_read_string(dst_addr, dst, sizeof(dst)) < 0) {
        return _EFAULT;
    }
    
    if (link(src, dst) < 0) {
        return -errno;
    }
    
    return 0;
}

dword_t sys_unlink(addr_t path_addr) {
    char path[4096];
    
    if (user_read_string(path_addr, path, sizeof(path)) < 0) {
        return _EFAULT;
    }
    
    if (unlink(path) < 0) {
        return -errno;
    }
    
    return 0;
}
