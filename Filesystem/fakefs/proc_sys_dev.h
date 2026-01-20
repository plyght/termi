#ifndef TERMI_PROC_SYS_DEV_H
#define TERMI_PROC_SYS_DEV_H

#include <sys/types.h>
#include <sys/stat.h>

int procfs_init(const char *mount_point);
int sysfs_init(const char *mount_point);
int devfs_init(const char *mount_point);

int procfs_stat(const char *path, struct stat *buf);
int sysfs_stat(const char *path, struct stat *buf);
int devfs_stat(const char *path, struct stat *buf);

ssize_t procfs_read(const char *path, char *buf, size_t size);
ssize_t sysfs_read(const char *path, char *buf, size_t size);
ssize_t devfs_read(const char *path, char *buf, size_t size);

ssize_t devfs_write(const char *path, const char *buf, size_t size);

#endif
