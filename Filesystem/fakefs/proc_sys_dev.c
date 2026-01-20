#include "proc_sys_dev.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

static char proc_mount[256];
static char sys_mount[256];
static char dev_mount[256];

int procfs_init(const char *mount_point) {
    strncpy(proc_mount, mount_point, sizeof(proc_mount) - 1);
    mkdir(proc_mount, 0755);
    
    char path[512];
    snprintf(path, sizeof(path), "%s/self", proc_mount);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/sys", proc_mount);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/sys/kernel", proc_mount);
    mkdir(path, 0755);
    
    return 0;
}

int sysfs_init(const char *mount_point) {
    strncpy(sys_mount, mount_point, sizeof(sys_mount) - 1);
    mkdir(sys_mount, 0755);
    
    char path[512];
    snprintf(path, sizeof(path), "%s/devices", sys_mount);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/class", sys_mount);
    mkdir(path, 0755);
    
    return 0;
}

int devfs_init(const char *mount_point) {
    strncpy(dev_mount, mount_point, sizeof(dev_mount) - 1);
    mkdir(dev_mount, 0755);
    
    char path[512];
    snprintf(path, sizeof(path), "%s/pts", dev_mount);
    mkdir(path, 0755);
    
    snprintf(path, sizeof(path), "%s/null", dev_mount);
    close(open(path, O_CREAT | O_WRONLY, 0666));
    snprintf(path, sizeof(path), "%s/zero", dev_mount);
    close(open(path, O_CREAT | O_WRONLY, 0666));
    snprintf(path, sizeof(path), "%s/random", dev_mount);
    close(open(path, O_CREAT | O_WRONLY, 0666));
    snprintf(path, sizeof(path), "%s/urandom", dev_mount);
    close(open(path, O_CREAT | O_WRONLY, 0666));
    
    return 0;
}

int procfs_stat(const char *path, struct stat *buf) {
    memset(buf, 0, sizeof(*buf));
    buf->st_mode = S_IFREG | 0444;
    buf->st_nlink = 1;
    buf->st_uid = 0;
    buf->st_gid = 0;
    buf->st_size = 0;
    buf->st_atime = buf->st_mtime = buf->st_ctime = time(NULL);
    return 0;
}

int sysfs_stat(const char *path, struct stat *buf) {
    return procfs_stat(path, buf);
}

int devfs_stat(const char *path, struct stat *buf) {
    memset(buf, 0, sizeof(*buf));
    
    if (strstr(path, "null") || strstr(path, "zero") || 
        strstr(path, "random") || strstr(path, "urandom")) {
        buf->st_mode = S_IFCHR | 0666;
        buf->st_rdev = 1;
    } else if (strstr(path, "tty")) {
        buf->st_mode = S_IFCHR | 0666;
        buf->st_rdev = 5;
    } else {
        buf->st_mode = S_IFREG | 0666;
    }
    
    buf->st_nlink = 1;
    buf->st_uid = 0;
    buf->st_gid = 0;
    buf->st_size = 0;
    buf->st_atime = buf->st_mtime = buf->st_ctime = time(NULL);
    return 0;
}

ssize_t procfs_read(const char *path, char *buf, size_t size) {
    if (strstr(path, "version")) {
        const char *version = "Linux version 5.15.0-termi (termi@ios) (gcc version 11.2.0) #1 SMP\n";
        size_t len = strlen(version);
        if (len > size) len = size;
        memcpy(buf, version, len);
        return len;
    }
    
    if (strstr(path, "cpuinfo")) {
        const char *cpuinfo = 
            "processor\t: 0\n"
            "vendor_id\t: ARM\n"
            "cpu family\t: 8\n"
            "model\t\t: 0\n"
            "model name\t: ARMv8 Processor\n"
            "stepping\t: 0\n"
            "cpu MHz\t\t: 2400.000\n"
            "cache size\t: 512 KB\n";
        size_t len = strlen(cpuinfo);
        if (len > size) len = size;
        memcpy(buf, cpuinfo, len);
        return len;
    }
    
    if (strstr(path, "meminfo")) {
        const char *meminfo = 
            "MemTotal:        2048000 kB\n"
            "MemFree:         1024000 kB\n"
            "MemAvailable:    1536000 kB\n"
            "Buffers:          102400 kB\n"
            "Cached:           512000 kB\n";
        size_t len = strlen(meminfo);
        if (len > size) len = size;
        memcpy(buf, meminfo, len);
        return len;
    }
    
    if (strstr(path, "uptime")) {
        char uptime[64];
        snprintf(uptime, sizeof(uptime), "%.2f %.2f\n", 
                (double)clock() / CLOCKS_PER_SEC, 0.0);
        size_t len = strlen(uptime);
        if (len > size) len = size;
        memcpy(buf, uptime, len);
        return len;
    }
    
    return 0;
}

ssize_t sysfs_read(const char *path, char *buf, size_t size) {
    return 0;
}

ssize_t devfs_read(const char *path, char *buf, size_t size) {
    if (strstr(path, "null")) {
        return 0;
    }
    
    if (strstr(path, "zero")) {
        memset(buf, 0, size);
        return size;
    }
    
    if (strstr(path, "random") || strstr(path, "urandom")) {
        for (size_t i = 0; i < size; i++) {
            buf[i] = rand() & 0xFF;
        }
        return size;
    }
    
    return 0;
}

ssize_t devfs_write(const char *path, const char *buf, size_t size) {
    if (strstr(path, "null")) {
        return size;
    }
    
    return -1;
}
