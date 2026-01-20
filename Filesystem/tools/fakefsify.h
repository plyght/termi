#ifndef TERMI_FAKEFSIFY_H
#define TERMI_FAKEFSIFY_H

#include <stdbool.h>

struct fakefsify_error {
    int line;
    enum {
        ERR_ARCHIVE,
        ERR_SQLITE,
        ERR_POSIX,
        ERR_CANCELLED,
    } type;
    int code;
    char *message;
};

struct progress {
    void *cookie;
    void (*callback)(void *cookie, double progress, const char *message, bool *cancel_out);
};

bool fakefs_import(const char *archive_path, const char *fs_path, 
                   struct fakefsify_error *err_out, struct progress progress);
bool fakefs_export(const char *fs_path, const char *archive_path, 
                   struct fakefsify_error *err_out, struct progress progress);

#endif
