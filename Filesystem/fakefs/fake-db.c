#include "fake-db.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void db_check_error(struct fakefs_db *fs)
{
    int errcode = sqlite3_errcode(fs->db);
    switch (errcode) {
    case SQLITE_OK:
    case SQLITE_ROW:
    case SQLITE_DONE:
        break;
    default:
        fprintf(stderr, "sqlite error: %d %#x %s\n", errcode, sqlite3_extended_errcode(fs->db),
                sqlite3_errmsg(fs->db));
        exit(1);
    }
}

static sqlite3_stmt *db_prepare(struct fakefs_db *fs, const char *stmt)
{
    sqlite3_stmt *statement;
    sqlite3_prepare_v2(fs->db, stmt, strlen(stmt) + 1, &statement, NULL);
    db_check_error(fs);
    return statement;
}

bool db_exec(struct fakefs_db *fs, sqlite3_stmt *stmt)
{
    int err = sqlite3_step(stmt);
    db_check_error(fs);
    return err == SQLITE_ROW;
}

void db_reset(struct fakefs_db *fs, sqlite3_stmt *stmt)
{
    sqlite3_reset(stmt);
    db_check_error(fs);
}

void db_exec_reset(struct fakefs_db *fs, sqlite3_stmt *stmt)
{
    db_exec(fs, stmt);
    db_reset(fs, stmt);
}

void db_begin_read(struct fakefs_db *fs)
{
    sqlite3_mutex_enter(fs->lock);
    db_exec_reset(fs, fs->stmt.begin_deferred);
}

void db_begin_write(struct fakefs_db *fs)
{
    sqlite3_mutex_enter(fs->lock);
    db_exec_reset(fs, fs->stmt.begin_immediate);
}

void db_commit(struct fakefs_db *fs)
{
    db_exec_reset(fs, fs->stmt.commit);
    sqlite3_mutex_leave(fs->lock);
}

void db_rollback(struct fakefs_db *fs)
{
    db_exec_reset(fs, fs->stmt.rollback);
    sqlite3_mutex_leave(fs->lock);
}

static void bind_path(sqlite3_stmt *stmt, int i, const char *path)
{
    sqlite3_bind_blob(stmt, i, path, strlen(path), SQLITE_TRANSIENT);
}

inode_t path_get_inode(struct fakefs_db *fs, const char *path)
{
    bind_path(fs->stmt.path_get_inode, 1, path);
    inode_t inode = 0;
    if (db_exec(fs, fs->stmt.path_get_inode))
        inode = sqlite3_column_int64(fs->stmt.path_get_inode, 0);
    db_reset(fs, fs->stmt.path_get_inode);
    return inode;
}

bool path_read_stat(struct fakefs_db *fs, const char *path, struct ish_stat *stat, inode_t *inode)
{
    bind_path(fs->stmt.path_read_stat, 1, path);
    bool exists = db_exec(fs, fs->stmt.path_read_stat);
    if (exists) {
        if (inode)
            *inode = sqlite3_column_int64(fs->stmt.path_read_stat, 0);
        if (stat)
            *stat = *(struct ish_stat *)sqlite3_column_blob(fs->stmt.path_read_stat, 1);
    }
    db_reset(fs, fs->stmt.path_read_stat);
    return exists;
}

inode_t path_create(struct fakefs_db *fs, const char *path, struct ish_stat *stat)
{
    sqlite3_bind_blob(fs->stmt.path_create_stat, 1, stat, sizeof(*stat), SQLITE_TRANSIENT);
    db_exec_reset(fs, fs->stmt.path_create_stat);
    inode_t inode = sqlite3_last_insert_rowid(fs->db);
    bind_path(fs->stmt.path_create_path, 1, path);
    db_exec_reset(fs, fs->stmt.path_create_path);
    return inode;
}

bool inode_read_stat_if_exist(struct fakefs_db *fs, inode_t inode, struct ish_stat *stat)
{
    sqlite3_bind_int64(fs->stmt.inode_read_stat, 1, inode);
    bool exist = db_exec(fs, fs->stmt.inode_read_stat);
    if (exist)
        *stat = *(struct ish_stat *)sqlite3_column_blob(fs->stmt.inode_read_stat, 0);
    db_reset(fs, fs->stmt.inode_read_stat);
    return exist;
}

void inode_read_stat_or_die(struct fakefs_db *fs, inode_t inode, struct ish_stat *stat)
{
    if (!inode_read_stat_if_exist(fs, inode, stat)) {
        fprintf(stderr, "inode_read_stat(%llu): missing inode\n", (unsigned long long)inode);
        exit(1);
    }
}

void inode_write_stat(struct fakefs_db *fs, inode_t inode, struct ish_stat *stat)
{
    sqlite3_bind_blob(fs->stmt.inode_write_stat, 1, stat, sizeof(*stat), SQLITE_TRANSIENT);
    sqlite3_bind_int64(fs->stmt.inode_write_stat, 2, inode);
    db_exec_reset(fs, fs->stmt.inode_write_stat);
}

void path_link(struct fakefs_db *fs, const char *src, const char *dst)
{
    inode_t inode = path_get_inode(fs, src);
    if (inode == 0) {
        fprintf(stderr, "fakefs link(%s, %s): nonexistent src path\n", src, dst);
        exit(1);
    }
    bind_path(fs->stmt.path_link, 1, dst);
    sqlite3_bind_int64(fs->stmt.path_link, 2, inode);
    db_exec_reset(fs, fs->stmt.path_link);
}

inode_t path_unlink(struct fakefs_db *fs, const char *path)
{
    inode_t inode = path_get_inode(fs, path);
    if (inode == 0) {
        fprintf(stderr, "path_unlink(%s): nonexistent path\n", path);
        exit(1);
    }
    bind_path(fs->stmt.path_unlink, 1, path);
    db_exec_reset(fs, fs->stmt.path_unlink);
    return inode;
}

void path_rename(struct fakefs_db *fs, const char *src, const char *dst)
{
    size_t src_len = strlen(src);
    sqlite3_bind_int64(fs->stmt.path_rename, 1, src_len);
    bind_path(fs->stmt.path_rename, 2, dst);
    char src_extra[src_len + 1];
    memcpy(src_extra, src, src_len);
    src_extra[src_len] = '/';
    sqlite3_bind_blob(fs->stmt.path_rename, 3, src_extra, src_len + 1, SQLITE_TRANSIENT);
    src_extra[src_len] = '0';
    sqlite3_bind_blob(fs->stmt.path_rename, 4, src_extra, src_len + 1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(fs->stmt.path_rename, 5, src_extra, src_len, SQLITE_TRANSIENT);
    db_exec_reset(fs, fs->stmt.path_rename);
}

static void sqlite_func_change_prefix(sqlite3_context *context, int argc, sqlite3_value **args)
{
    if (argc != 3)
        return;
    const void *in_blob = sqlite3_value_blob(args[0]);
    size_t in_size = sqlite3_value_bytes(args[0]);
    size_t start = sqlite3_value_int64(args[1]);
    const void *replacement = sqlite3_value_blob(args[2]);
    size_t replacement_size = sqlite3_value_bytes(args[2]);
    size_t out_size = in_size - start + replacement_size;
    char *out_blob = sqlite3_malloc(out_size);
    memcpy(out_blob, replacement, replacement_size);
    memcpy(out_blob + replacement_size, in_blob + start, in_size - start);
    sqlite3_result_blob(context, out_blob, out_size, sqlite3_free);
}

static const char *db_schema =
    "CREATE TABLE meta (id INTEGER UNIQUE DEFAULT 0, db_inode INTEGER);"
    "INSERT INTO meta (db_inode) VALUES (0);"
    "CREATE TABLE stats (inode INTEGER PRIMARY KEY, stat BLOB);"
    "CREATE TABLE paths (path BLOB PRIMARY KEY, inode INTEGER REFERENCES stats(inode));"
    "CREATE INDEX inode_to_path ON paths (inode, path);"
    "PRAGMA user_version=3;";

int fake_db_init(struct fakefs_db *fs, const char *db_path, const char *root_path)
{
    int err = sqlite3_open_v2(db_path, &fs->db, SQLITE_OPEN_READWRITE, NULL);
    if (err != SQLITE_OK) {
        fprintf(stderr, "error opening database: %s\n", sqlite3_errmsg(fs->db));
        sqlite3_close(fs->db);
        return -1;
    }

    sqlite3_busy_timeout(fs->db, 1000);
    sqlite3_create_function(fs->db, "change_prefix", 3, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL,
                            sqlite_func_change_prefix, NULL, NULL);
    db_check_error(fs);

    sqlite3_stmt *statement = db_prepare(fs, "PRAGMA journal_mode=wal");
    sqlite3_step(statement);
    db_check_error(fs);
    sqlite3_finalize(statement);

    statement = db_prepare(fs, "PRAGMA foreign_keys=true");
    sqlite3_step(statement);
    db_check_error(fs);
    sqlite3_finalize(statement);

    struct stat statbuf;
    if (stat(db_path, &statbuf) < 0) {
        perror("stat database");
        return -1;
    }
    ino_t db_inode = statbuf.st_ino;

    statement = db_prepare(fs, "SELECT db_inode FROM meta");
    if (sqlite3_step(statement) == SQLITE_ROW) {
        if ((uint64_t)sqlite3_column_int64(statement, 0) != db_inode) {
            fprintf(stderr, "Database inode mismatch - rebuild needed\n");
        }
    }
    sqlite3_finalize(statement);

    statement = db_prepare(fs, "UPDATE meta SET db_inode = ?");
    sqlite3_bind_int64(statement, 1, (int64_t)db_inode);
    sqlite3_step(statement);
    db_check_error(fs);
    sqlite3_finalize(statement);

    statement = db_prepare(fs, "DELETE FROM stats WHERE NOT EXISTS "
                               "(SELECT 1 FROM paths WHERE inode = stats.inode)");
    sqlite3_step(statement);
    db_check_error(fs);
    sqlite3_finalize(statement);

    fs->lock = sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);
    fs->stmt.begin_deferred = db_prepare(fs, "BEGIN DEFERRED");
    fs->stmt.begin_immediate = db_prepare(fs, "BEGIN IMMEDIATE");
    fs->stmt.commit = db_prepare(fs, "COMMIT");
    fs->stmt.rollback = db_prepare(fs, "ROLLBACK");
    fs->stmt.path_get_inode = db_prepare(fs, "SELECT inode FROM paths WHERE path = ?");
    fs->stmt.path_read_stat =
        db_prepare(fs, "SELECT inode, stat FROM stats NATURAL JOIN paths WHERE path = ?");
    fs->stmt.path_create_stat = db_prepare(fs, "INSERT INTO stats (stat) VALUES (?)");
    fs->stmt.path_create_path =
        db_prepare(fs, "INSERT OR REPLACE INTO paths VALUES (?, last_insert_rowid())");
    fs->stmt.inode_read_stat = db_prepare(fs, "SELECT stat FROM stats WHERE inode = ?");
    fs->stmt.inode_write_stat = db_prepare(fs, "UPDATE stats SET stat = ? WHERE inode = ?");
    fs->stmt.path_link = db_prepare(fs, "INSERT OR REPLACE INTO paths (path, inode) VALUES (?, ?)");
    fs->stmt.path_unlink = db_prepare(fs, "DELETE FROM paths WHERE path = ?");
    fs->stmt.path_rename =
        db_prepare(fs, "UPDATE OR REPLACE paths SET path = change_prefix(path, ?, ?) "
                       "WHERE (path >= ? AND path < ?) OR path = ?");
    fs->stmt.path_from_inode = db_prepare(fs, "SELECT path FROM paths WHERE inode = ?");
    fs->stmt.try_cleanup_inode = db_prepare(fs, "DELETE FROM stats WHERE inode = ? AND NOT EXISTS "
                                                "(SELECT 1 FROM paths WHERE inode = stats.inode)");
    return 0;
}

int fake_db_deinit(struct fakefs_db *fs)
{
    if (fs->db) {
        sqlite3_finalize(fs->stmt.begin_deferred);
        sqlite3_finalize(fs->stmt.begin_immediate);
        sqlite3_finalize(fs->stmt.commit);
        sqlite3_finalize(fs->stmt.rollback);
        sqlite3_finalize(fs->stmt.path_get_inode);
        sqlite3_finalize(fs->stmt.path_read_stat);
        sqlite3_finalize(fs->stmt.path_create_stat);
        sqlite3_finalize(fs->stmt.path_create_path);
        sqlite3_finalize(fs->stmt.inode_read_stat);
        sqlite3_finalize(fs->stmt.inode_write_stat);
        sqlite3_finalize(fs->stmt.path_link);
        sqlite3_finalize(fs->stmt.path_unlink);
        sqlite3_finalize(fs->stmt.path_rename);
        sqlite3_finalize(fs->stmt.path_from_inode);
        sqlite3_finalize(fs->stmt.try_cleanup_inode);
        sqlite3_mutex_free(fs->lock);
        return sqlite3_close(fs->db);
    }
    return SQLITE_OK;
}
