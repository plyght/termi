#include "fake.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *fix_path(const char *path)
{
    if (path[0] == '/')
        return path + 1;
    return path;
}

void ish_stat_to_stat(const struct ish_stat *ish, struct stat *st, inode_t inode)
{
    memset(st, 0, sizeof(*st));
    st->st_ino = inode;
    st->st_mode = ish->mode;
    st->st_uid = ish->uid;
    st->st_gid = ish->gid;
    st->st_rdev = ish->rdev;
    st->st_nlink = 1;
}

void stat_to_ish_stat(const struct stat *st, struct ish_stat *ish)
{
    ish->mode = st->st_mode;
    ish->uid = st->st_uid;
    ish->gid = st->st_gid;
    ish->rdev = st->st_rdev;
}

int fakefs_init(struct fakefs *fs, const char *db_path, const char *root_path)
{
    fs->root_path = strdup(root_path);
    fs->root_fd = open(root_path, O_RDONLY);
    if (fs->root_fd < 0) {
        perror("open root path");
        free(fs->root_path);
        return -1;
    }

    return fake_db_init(&fs->db, db_path, root_path);
}

int fakefs_deinit(struct fakefs *fs)
{
    int ret = fake_db_deinit(&fs->db);
    close(fs->root_fd);
    free(fs->root_path);
    return ret;
}

int fakefs_open(struct fakefs *fs, const char *path, int flags, mode_t mode)
{
    db_begin_write(&fs->db);

    int fd = openat(fs->root_fd, fix_path(path), flags, 0666);
    if (fd < 0) {
        db_rollback(&fs->db);
        return -1;
    }

    inode_t inode = path_get_inode(&fs->db, path);
    if (flags & O_CREAT) {
        if (inode == 0) {
            struct ish_stat ishstat;
            ishstat.mode = mode | S_IFREG;
            ishstat.uid = getuid();
            ishstat.gid = getgid();
            ishstat.rdev = 0;
            path_create(&fs->db, path, &ishstat);
        }
    }

    db_commit(&fs->db);

    if (inode == 0 && !(flags & O_CREAT)) {
        close(fd);
        errno = ENOENT;
        return -1;
    }

    return fd;
}

ssize_t fakefs_read(struct fakefs *fs, int fd, void *buf, size_t count)
{
    return read(fd, buf, count);
}

ssize_t fakefs_write(struct fakefs *fs, int fd, const void *buf, size_t count)
{
    return write(fd, buf, count);
}

int fakefs_close(struct fakefs *fs, int fd)
{
    return close(fd);
}

int fakefs_stat(struct fakefs *fs, const char *path, struct stat *statbuf)
{
    db_begin_read(&fs->db);

    struct ish_stat ishstat;
    inode_t inode;
    if (!path_read_stat(&fs->db, path, &ishstat, &inode)) {
        db_rollback(&fs->db);
        errno = ENOENT;
        return -1;
    }

    if (fstatat(fs->root_fd, fix_path(path), statbuf, 0) < 0) {
        db_rollback(&fs->db);
        return -1;
    }

    db_commit(&fs->db);

    statbuf->st_ino = inode;
    statbuf->st_mode = ishstat.mode;
    statbuf->st_uid = ishstat.uid;
    statbuf->st_gid = ishstat.gid;
    statbuf->st_rdev = ishstat.rdev;

    return 0;
}

int fakefs_lstat(struct fakefs *fs, const char *path, struct stat *statbuf)
{
    db_begin_read(&fs->db);

    struct ish_stat ishstat;
    inode_t inode;
    if (!path_read_stat(&fs->db, path, &ishstat, &inode)) {
        db_rollback(&fs->db);
        errno = ENOENT;
        return -1;
    }

    if (fstatat(fs->root_fd, fix_path(path), statbuf, AT_SYMLINK_NOFOLLOW) < 0) {
        db_rollback(&fs->db);
        return -1;
    }

    db_commit(&fs->db);

    statbuf->st_ino = inode;
    statbuf->st_mode = ishstat.mode;
    statbuf->st_uid = ishstat.uid;
    statbuf->st_gid = ishstat.gid;
    statbuf->st_rdev = ishstat.rdev;

    return 0;
}

int fakefs_mkdir(struct fakefs *fs, const char *path, mode_t mode)
{
    db_begin_write(&fs->db);

    if (mkdirat(fs->root_fd, fix_path(path), 0777) < 0) {
        db_rollback(&fs->db);
        return -1;
    }

    struct ish_stat ishstat;
    ishstat.mode = mode | S_IFDIR;
    ishstat.uid = getuid();
    ishstat.gid = getgid();
    ishstat.rdev = 0;
    path_create(&fs->db, path, &ishstat);

    db_commit(&fs->db);
    return 0;
}

int fakefs_unlink(struct fakefs *fs, const char *path)
{
    db_begin_write(&fs->db);

    if (unlinkat(fs->root_fd, fix_path(path), 0) < 0) {
        db_rollback(&fs->db);
        return -1;
    }

    path_unlink(&fs->db, path);
    db_commit(&fs->db);
    return 0;
}

int fakefs_rmdir(struct fakefs *fs, const char *path)
{
    db_begin_write(&fs->db);

    if (unlinkat(fs->root_fd, fix_path(path), AT_REMOVEDIR) < 0) {
        db_rollback(&fs->db);
        return -1;
    }

    path_unlink(&fs->db, path);
    db_commit(&fs->db);
    return 0;
}

int fakefs_rename(struct fakefs *fs, const char *oldpath, const char *newpath)
{
    db_begin_write(&fs->db);

    path_rename(&fs->db, oldpath, newpath);

    if (renameat(fs->root_fd, fix_path(oldpath), fs->root_fd, fix_path(newpath)) < 0) {
        db_rollback(&fs->db);
        return -1;
    }

    db_commit(&fs->db);
    return 0;
}

int fakefs_symlink(struct fakefs *fs, const char *target, const char *linkpath)
{
    db_begin_write(&fs->db);

    int fd = openat(fs->root_fd, fix_path(linkpath), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd < 0) {
        db_rollback(&fs->db);
        return -1;
    }

    ssize_t res = write(fd, target, strlen(target));
    close(fd);

    if (res < 0) {
        unlinkat(fs->root_fd, fix_path(linkpath), 0);
        db_rollback(&fs->db);
        return -1;
    }

    struct ish_stat ishstat;
    ishstat.mode = S_IFLNK | 0777;
    ishstat.uid = getuid();
    ishstat.gid = getgid();
    ishstat.rdev = 0;
    path_create(&fs->db, linkpath, &ishstat);

    db_commit(&fs->db);
    return 0;
}

ssize_t fakefs_readlink(struct fakefs *fs, const char *path, char *buf, size_t bufsiz)
{
    db_begin_read(&fs->db);

    struct ish_stat ishstat;
    if (!path_read_stat(&fs->db, path, &ishstat, NULL)) {
        db_rollback(&fs->db);
        errno = ENOENT;
        return -1;
    }

    if (!S_ISLNK(ishstat.mode)) {
        db_rollback(&fs->db);
        errno = EINVAL;
        return -1;
    }

    int fd = openat(fs->root_fd, fix_path(path), O_RDONLY);
    if (fd < 0) {
        db_rollback(&fs->db);
        return -1;
    }

    ssize_t len = read(fd, buf, bufsiz);
    close(fd);

    db_commit(&fs->db);
    return len;
}

int fakefs_chmod(struct fakefs *fs, const char *path, mode_t mode)
{
    db_begin_write(&fs->db);

    struct ish_stat ishstat;
    inode_t inode;
    if (!path_read_stat(&fs->db, path, &ishstat, &inode)) {
        db_rollback(&fs->db);
        errno = ENOENT;
        return -1;
    }

    ishstat.mode = (ishstat.mode & S_IFMT) | (mode & ~S_IFMT);
    inode_write_stat(&fs->db, inode, &ishstat);

    db_commit(&fs->db);
    return 0;
}

int fakefs_chown(struct fakefs *fs, const char *path, uid_t owner, gid_t group)
{
    db_begin_write(&fs->db);

    struct ish_stat ishstat;
    inode_t inode;
    if (!path_read_stat(&fs->db, path, &ishstat, &inode)) {
        db_rollback(&fs->db);
        errno = ENOENT;
        return -1;
    }

    if (owner != (uid_t)-1)
        ishstat.uid = owner;
    if (group != (gid_t)-1)
        ishstat.gid = group;

    inode_write_stat(&fs->db, inode, &ishstat);

    db_commit(&fs->db);
    return 0;
}

DIR *fakefs_opendir(struct fakefs *fs, const char *path)
{
    int fd = openat(fs->root_fd, fix_path(path), O_RDONLY | O_DIRECTORY);
    if (fd < 0)
        return NULL;
    return fdopendir(fd);
}

struct dirent *fakefs_readdir(struct fakefs *fs, DIR *dirp)
{
    return readdir(dirp);
}

int fakefs_closedir(struct fakefs *fs, DIR *dirp)
{
    return closedir(dirp);
}
