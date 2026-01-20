#ifndef TERMI_TYPES_H
#define TERMI_TYPES_H

#include <stdint.h>
#include <sys/types.h>

typedef int32_t dword_t;
typedef uint32_t addr_t;
typedef int32_t pid_t_;
typedef uint32_t uid_t_;
typedef uint32_t gid_t_;
typedef uint32_t mode_t_;
typedef int32_t fd_t;
typedef int32_t int_t;
typedef uint32_t uint_t;
typedef uint16_t word_t;
typedef int64_t off_t_;
typedef uint32_t dev_t_;

#define _EPERM 1
#define _ENOENT 2
#define _ESRCH 3
#define _EINTR 4
#define _EIO 5
#define _ENXIO 6
#define _E2BIG 7
#define _ENOEXEC 8
#define _EBADF 9
#define _ECHILD 10
#define _EAGAIN 11
#define _ENOMEM 12
#define _EACCES 13
#define _EFAULT 14
#define _ENOTBLK 15
#define _EBUSY 16
#define _EEXIST 17
#define _EXDEV 18
#define _ENODEV 19
#define _ENOTDIR 20
#define _EISDIR 21
#define _EINVAL 22
#define _ENFILE 23
#define _EMFILE 24
#define _ENOTTY 25
#define _ETXTBSY 26
#define _EFBIG 27
#define _ENOSPC 28
#define _ESPIPE 29
#define _EROFS 30
#define _EMLINK 31
#define _EPIPE 32
#define _EDOM 33
#define _ERANGE 34
#define _ENOSYS 38

#define must_check __attribute__((warn_unused_result))

#endif
