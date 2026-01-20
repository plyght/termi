#include "calls.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define PROT_READ_ 0x1
#define PROT_WRITE_ 0x2
#define PROT_EXEC_ 0x4
#define PROT_NONE_ 0x0

#define MAP_SHARED_ 0x01
#define MAP_PRIVATE_ 0x02
#define MAP_FIXED_ 0x10
#define MAP_ANONYMOUS_ 0x20

static int translate_prot(int_t linux_prot)
{
    int darwin_prot = 0;

    if (linux_prot & PROT_READ_)
        darwin_prot |= PROT_READ;
    if (linux_prot & PROT_WRITE_)
        darwin_prot |= PROT_WRITE;
    if (linux_prot & PROT_EXEC_)
        darwin_prot |= PROT_EXEC;

    return darwin_prot;
}

static int translate_mmap_flags(dword_t linux_flags)
{
    int darwin_flags = 0;

    if (linux_flags & MAP_SHARED_)
        darwin_flags |= MAP_SHARED;
    if (linux_flags & MAP_PRIVATE_)
        darwin_flags |= MAP_PRIVATE;
    if (linux_flags & MAP_FIXED_)
        darwin_flags |= MAP_FIXED;
    if (linux_flags & MAP_ANONYMOUS_)
        darwin_flags |= MAP_ANON;

    return darwin_flags;
}

static void *brk_current = NULL;
static void *brk_base = NULL;

addr_t sys_brk(addr_t new_brk)
{
    if (brk_base == NULL) {
        brk_base =
            mmap(NULL, 1024 * 1024 * 64, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (brk_base == MAP_FAILED) {
            return 0;
        }
        brk_current = brk_base;
    }

    if (new_brk == 0) {
        return (addr_t)(uintptr_t)brk_current;
    }

    void *requested = (void *)(uintptr_t)new_brk;
    void *brk_end = (char *)brk_base + 1024 * 1024 * 64;
    if (requested < brk_base || requested > brk_end) {
        return (addr_t)(uintptr_t)brk_current;
    }

    brk_current = requested;
    return (addr_t)(uintptr_t)brk_current;
}

addr_t sys_mmap2(addr_t addr, dword_t len, dword_t prot, dword_t flags, fd_t fd_no, dword_t offset)
{
    void *hint = addr ? (void *)(uintptr_t)addr : NULL;
    int darwin_prot = translate_prot(prot);
    int darwin_flags = translate_mmap_flags(flags);

    off_t file_offset = (off_t)offset * 4096;

    void *result = mmap(hint, len, darwin_prot, darwin_flags, fd_no, file_offset);

    if (result == MAP_FAILED) {
        return -errno;
    }

    return (addr_t)(uintptr_t)result;
}

int_t sys_munmap(addr_t addr, uint_t len)
{
    void *ptr = (void *)(uintptr_t)addr;

    if (munmap(ptr, len) < 0) {
        return -errno;
    }

    return 0;
}

int_t sys_mprotect(addr_t addr, uint_t len, int_t prot)
{
    void *ptr = (void *)(uintptr_t)addr;
    int darwin_prot = translate_prot(prot);

    if (mprotect(ptr, len, darwin_prot) < 0) {
        return -errno;
    }

    return 0;
}
