#ifndef TERMI_FAKE_H
#define TERMI_FAKE_H

#include "fake-db.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

struct fakefs {
    struct fakefs_db db;
    char *root_path;
    int root_fd;
};

int fakefs_init(struct fakefs *fs, const char *db_path, const char *root_path);
int fakefs_deinit(struct fakefs *fs);

int fakefs_open(struct fakefs *fs, const char *path, int flags, mode_t mode);
ssize_t fakefs_read(struct fakefs *fs, int fd, void *buf, size_t count);
ssize_t fakefs_write(struct fakefs *fs, int fd, const void *buf, size_t count);
int fakefs_close(struct fakefs *fs, int fd);

int fakefs_stat(struct fakefs *fs, const char *path, struct stat *statbuf);
int fakefs_lstat(struct fakefs *fs, const char *path, struct stat *statbuf);
int fakefs_fstat(struct fakefs *fs, int fd, struct stat *statbuf);

int fakefs_mkdir(struct fakefs *fs, const char *path, mode_t mode);
int fakefs_rmdir(struct fakefs *fs, const char *path);
int fakefs_unlink(struct fakefs *fs, const char *path);
int fakefs_rename(struct fakefs *fs, const char *oldpath, const char *newpath);

int fakefs_symlink(struct fakefs *fs, const char *target, const char *linkpath);
ssize_t fakefs_readlink(struct fakefs *fs, const char *path, char *buf, size_t bufsiz);

int fakefs_chmod(struct fakefs *fs, const char *path, mode_t mode);
int fakefs_chown(struct fakefs *fs, const char *path, uid_t owner, gid_t group);

DIR *fakefs_opendir(struct fakefs *fs, const char *path);
struct dirent *fakefs_readdir(struct fakefs *fs, DIR *dirp);
int fakefs_closedir(struct fakefs *fs, DIR *dirp);

void ish_stat_to_stat(const struct ish_stat *ish, struct stat *st, inode_t inode);
void stat_to_ish_stat(const struct stat *st, struct ish_stat *ish);

#endif
