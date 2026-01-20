#include "fakefsify.h"
#include "../fakefs/fake-db.h"
#include <archive.h>
#include <archive_entry.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define FILL_ERR(_type, _code, _message)                                                           \
    do {                                                                                           \
        err_out->line = __LINE__;                                                                  \
        err_out->type = _type;                                                                     \
        err_out->code = _code;                                                                     \
        err_out->message = strdup(_message);                                                       \
        return false;                                                                              \
    } while (0)

#define ARCHIVE_ERR(archive)                                                                       \
    FILL_ERR(ERR_ARCHIVE, archive_errno(archive), archive_error_string(archive))
#define POSIX_ERR() FILL_ERR(ERR_POSIX, errno, strerror(errno))
#define SQLITE_ERR(db) FILL_ERR(ERR_SQLITE, sqlite3_extended_errcode(db), sqlite3_errmsg(db))
#define CANCEL() FILL_ERR(ERR_CANCELLED, 0, "");

static bool progress_update(struct progress *p, double progress, const char *message)
{
    bool cancelled = false;
    if (p && p->callback)
        p->callback(p->cookie, progress, message, &cancelled);
    return !cancelled;
}

static bool path_normalize(const char *path, char *out)
{
#define ends_path(c) (c == '\0' || c == '/')
    while (path[0] != '\0') {
        while (path[0] == '/')
            path++;
        if (path[0] == '\0')
            break;
        if (path[0] == '.' && path[1] == '.' && ends_path(path[2]))
            return false;
        if (path[0] == '.' && ends_path(path[1])) {
            path++;
        } else {
            *out++ = '/';
            while (path[0] != '/' && path[0] != '\0')
                *out++ = *path++;
        }
    }
    *out = '\0';
    return true;
}

static const char *fix_path(const char *path)
{
    if (path[0] == '/')
        return path + 1;
    return path;
}

static const char *db_schema =
    "CREATE TABLE meta (id INTEGER UNIQUE DEFAULT 0, db_inode INTEGER);"
    "INSERT INTO meta (db_inode) VALUES (0);"
    "CREATE TABLE stats (inode INTEGER PRIMARY KEY, stat BLOB);"
    "CREATE TABLE paths (path BLOB PRIMARY KEY, inode INTEGER REFERENCES stats(inode));"
    "CREATE INDEX inode_to_path ON paths (inode, path);"
    "PRAGMA user_version=3;";

bool fakefs_import(const char *archive_path, const char *fs_path, struct fakefsify_error *err_out,
                   struct progress p)
{

    if (mkdir(fs_path, 0777) < 0)
        POSIX_ERR();

    char path_tmp[PATH_MAX];
    snprintf(path_tmp, sizeof(path_tmp), "%s/data", fs_path);
    if (mkdir(path_tmp, 0777) < 0)
        POSIX_ERR();

    int root_fd = open(path_tmp, O_RDONLY);
    if (root_fd < 0)
        POSIX_ERR();

    snprintf(path_tmp, sizeof(path_tmp), "%s/meta.db", fs_path);
    sqlite3 *db;
    int err = sqlite3_open_v2(path_tmp, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (err != SQLITE_OK)
        SQLITE_ERR(db);

    char *errmsg;
    sqlite3_exec(db, "PRAGMA journal_mode=wal", NULL, NULL, &errmsg);
    sqlite3_exec(db, "BEGIN", NULL, NULL, &errmsg);
    sqlite3_exec(db, db_schema, NULL, NULL, &errmsg);

    struct archive *archive = archive_read_new();
    if (archive == NULL)
        ARCHIVE_ERR(archive);
    archive_read_support_filter_gzip(archive);
    archive_read_support_format_tar(archive);
    if (archive_read_open_filename(archive, archive_path, 65536) != ARCHIVE_OK)
        ARCHIVE_ERR(archive);

    struct stat archive_stat;
    if (stat(archive_path, &archive_stat) < 0)
        POSIX_ERR();
    size_t archive_bytes = archive_stat.st_size;

    sqlite3_stmt *insert_stat;
    sqlite3_stmt *insert_path;
    sqlite3_stmt *insert_hardlink;

    sqlite3_prepare_v2(db, "INSERT INTO stats (stat) VALUES (?)", -1, &insert_stat, NULL);
    sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO paths VALUES (?, ?)", -1, &insert_path, NULL);
    sqlite3_prepare_v2(db,
                       "INSERT OR REPLACE INTO paths VALUES (?, "
                       "(SELECT inode FROM paths WHERE path = ? LIMIT 1))",
                       -1, &insert_hardlink, NULL);

    bool archive_has_root = false;

    struct archive_entry *entry;
    while ((err = archive_read_next_header(archive, &entry)) == ARCHIVE_OK) {
        char entry_path[PATH_MAX];
        if (!path_normalize(archive_entry_pathname(entry), entry_path)) {
            fprintf(stderr, "warning: skipped possible path traversal %s\n",
                    archive_entry_pathname(entry));
            continue;
        }

        if (!progress_update(&p, (double)archive_filter_bytes(archive, -1) / archive_bytes,
                             entry_path))
            CANCEL();

        if (strcmp(entry_path, "") == 0)
            archive_has_root = true;

        const char *hardlink = archive_entry_hardlink(entry);
        if (hardlink) {
            char hardlink_path[PATH_MAX];
            if (!path_normalize(hardlink, hardlink_path)) {
                fprintf(stderr, "warning: invalid hardlink %s\n", hardlink);
                continue;
            }
            if (linkat(root_fd, fix_path(hardlink_path), root_fd, fix_path(entry_path), 0) < 0)
                POSIX_ERR();
            sqlite3_bind_blob64(insert_hardlink, 1, entry_path, strlen(entry_path),
                                SQLITE_TRANSIENT);
            sqlite3_bind_blob64(insert_hardlink, 2, hardlink_path, strlen(hardlink_path),
                                SQLITE_TRANSIENT);
            sqlite3_step(insert_hardlink);
            sqlite3_reset(insert_hardlink);
            continue;
        }

        char *entry_path_copy = strdup(entry_path);
        char *slash = entry_path_copy;
        while ((slash = strchr(*slash ? slash + 1 : slash, '/')) != NULL) {
            *slash = '\0';
            int err = mkdirat(root_fd, fix_path(entry_path_copy), 0777);
            *slash = '/';
            if (err < 0 && errno != EEXIST)
                POSIX_ERR();
        }
        free(entry_path_copy);

        int fd = -1;
        switch (archive_entry_filetype(entry)) {
        case AE_IFREG:
        case AE_IFLNK:
        case AE_IFBLK:
        case AE_IFCHR:
        case AE_IFSOCK:
            fd = openat(root_fd, fix_path(entry_path), O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd < 0 && errno != EISDIR)
                POSIX_ERR();
            break;
        case AE_IFDIR:
            if (strcmp(entry_path, "") != 0) {
                err = mkdirat(root_fd, fix_path(entry_path), 0777);
                if (err < 0 && errno != EEXIST)
                    POSIX_ERR();
            }
            break;
        case AE_IFIFO:
            break;
        }

        switch (archive_entry_filetype(entry)) {
        case AE_IFREG:
            if (fd >= 0 && archive_read_data_into_fd(archive, fd) != ARCHIVE_OK)
                ARCHIVE_ERR(archive);
            break;
        case AE_IFLNK:
            if (fd >= 0) {
                const char *symlink_target = archive_entry_symlink(entry);
                if (write(fd, symlink_target, strlen(symlink_target)) < 0)
                    POSIX_ERR();
            }
            break;
        default:
            break;
        }

        if (fd != -1)
            close(fd);

        struct ish_stat stat = {
            .mode = (uint32_t)archive_entry_mode(entry),
            .uid = (uint32_t)archive_entry_uid(entry),
            .gid = (uint32_t)archive_entry_gid(entry),
            .rdev = (uint32_t)archive_entry_rdev(entry),
        };

        sqlite3_bind_blob64(insert_stat, 1, &stat, sizeof(stat), SQLITE_TRANSIENT);
        sqlite3_step(insert_stat);
        sqlite3_reset(insert_stat);

        sqlite3_bind_blob64(insert_path, 1, entry_path, strlen(entry_path), SQLITE_TRANSIENT);
        sqlite3_bind_int64(insert_path, 2, sqlite3_last_insert_rowid(db));
        sqlite3_step(insert_path);
        sqlite3_reset(insert_path);
    }

    if (err != ARCHIVE_EOF)
        ARCHIVE_ERR(archive);

    if (!archive_has_root) {
        struct ish_stat stat = {.mode = 0755 | S_IFDIR};
        sqlite3_bind_blob64(insert_stat, 1, &stat, sizeof(stat), SQLITE_TRANSIENT);
        sqlite3_step(insert_stat);
        sqlite3_reset(insert_stat);
        sqlite3_bind_blob64(insert_path, 1, "", 0, SQLITE_TRANSIENT);
        sqlite3_bind_int64(insert_path, 2, sqlite3_last_insert_rowid(db));
        sqlite3_step(insert_path);
        sqlite3_reset(insert_path);
    }

    sqlite3_finalize(insert_stat);
    sqlite3_finalize(insert_path);
    sqlite3_finalize(insert_hardlink);
    sqlite3_exec(db, "COMMIT", NULL, NULL, &errmsg);
    sqlite3_close(db);
    close(root_fd);
    archive_read_free(archive);

    return true;
}

bool fakefs_export(const char *fs_path, const char *archive_path, struct fakefsify_error *err_out,
                   struct progress p)
{

    struct archive *archive = archive_write_new();
    if (archive == NULL)
        ARCHIVE_ERR(archive);
    archive_write_add_filter_gzip(archive);
    archive_write_set_format_pax(archive);
    if (archive_write_open_filename(archive, archive_path) != ARCHIVE_OK)
        ARCHIVE_ERR(archive);

    char path_tmp[PATH_MAX];
    snprintf(path_tmp, sizeof(path_tmp), "%s/data", fs_path);
    int root_fd = open(path_tmp, O_RDONLY);
    if (root_fd < 0)
        POSIX_ERR();

    snprintf(path_tmp, sizeof(path_tmp), "%s/meta.db", fs_path);
    sqlite3 *db;
    int err = sqlite3_open_v2(path_tmp, &db, SQLITE_OPEN_READONLY, NULL);
    if (err != SQLITE_OK)
        SQLITE_ERR(db);

    char *errmsg;
    sqlite3_exec(db, "BEGIN", NULL, NULL, &errmsg);

    sqlite3_stmt *count_stmt;
    sqlite3_prepare_v2(db, "SELECT COUNT(path) FROM paths", -1, &count_stmt, NULL);
    sqlite3_step(count_stmt);
    int64_t paths_total = sqlite3_column_int64(count_stmt, 0);
    sqlite3_finalize(count_stmt);
    int64_t paths_done = 0;

    sqlite3_stmt *query;
    sqlite3_prepare_v2(db, "SELECT path, inode, stat FROM paths, stats USING (inode)", -1, &query,
                       NULL);

    while (sqlite3_step(query) == SQLITE_ROW) {
        struct archive_entry *entry = archive_entry_new();

        const char *path_in_db = sqlite3_column_blob(query, 0);
        size_t path_len = sqlite3_column_bytes(query, 0);
        char *path = malloc(path_len + 2);
        path[0] = '.';
        memcpy(path + 1, path_in_db, path_len);
        path[path_len + 1] = '\0';
        archive_entry_set_pathname(entry, path);

        if (!progress_update(&p, (double)paths_done / paths_total, path))
            CANCEL();

        archive_entry_set_ino64(entry, sqlite3_column_int64(query, 1));
        struct ish_stat stat = *(struct ish_stat *)sqlite3_column_blob(query, 2);
        archive_entry_set_mode(entry, stat.mode);
        archive_entry_set_uid(entry, stat.uid);
        archive_entry_set_gid(entry, stat.gid);
        archive_entry_set_rdev(entry, stat.rdev);

        struct stat real_stat;
        if (fstatat(root_fd, fix_path(path_in_db), &real_stat, 0) < 0) {
            if (errno == ENOENT) {
                printf("skipping %s\n", path);
                free(path);
                archive_entry_free(entry);
                paths_done++;
                continue;
            }
            POSIX_ERR();
        }

        archive_entry_set_size(entry, real_stat.st_size);

        int fd = -1;
        if (S_ISREG(stat.mode) || S_ISLNK(stat.mode))
            fd = openat(root_fd, fix_path(path_in_db), O_RDONLY);

        if (S_ISLNK(stat.mode) && fd >= 0) {
            char buf[PATH_MAX];
            ssize_t len = read(fd, buf, sizeof(buf) - 1);
            if (len >= 0) {
                buf[len] = '\0';
                archive_entry_set_symlink(entry, buf);
            }
        }

        archive_write_header(archive, entry);

        if (S_ISREG(stat.mode) && archive_entry_size(entry) != 0 && fd >= 0) {
            char buf[8192];
            ssize_t len;
            while ((len = read(fd, buf, sizeof(buf))) > 0) {
                archive_write_data(archive, buf, len);
            }
        }

        if (fd != -1)
            close(fd);

        paths_done++;
        free(path);
        archive_entry_free(entry);
    }

    sqlite3_finalize(query);
    sqlite3_close(db);
    close(root_fd);
    archive_write_close(archive);
    archive_write_free(archive);

    return true;
}

int main(int argc, const char *argv[])
{
    enum { cmd_import, cmd_export } cmd = cmd_import;

    if (argc > 0 && strcmp(basename((char *)argv[0]), "unfakefsify") == 0) {
        cmd = cmd_export;
    }

    if (argc != 3) {
        fprintf(stderr, "wrong number of arguments\n");
        if (cmd == cmd_import)
            fprintf(stderr, "usage: %s <rootfs.tar.gz> <fakefs>\n", argv[0]);
        else
            fprintf(stderr, "usage: %s <fakefs> <rootfs.tar.gz>\n", argv[0]);
        return 1;
    }

    struct fakefsify_error err;
    bool success;

    if (cmd == cmd_import)
        success = fakefs_import(argv[1], argv[2], &err, (struct progress){});
    else
        success = fakefs_export(argv[1], argv[2], &err, (struct progress){});

    if (!success) {
        fprintf(stderr, "error!!1! %d %d %s\n", err.line, err.type, err.message);
        return 1;
    }

    return 0;
}
