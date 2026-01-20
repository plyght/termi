#include <string.h>
#include <stdio.h>
#include "calls.h"

dword_t syscall_stub(void) {
    return _ENOSYS;
}

dword_t syscall_silent_stub(void) {
    return _ENOSYS;
}

dword_t syscall_success_stub(void) {
    return 0;
}

syscall_t syscall_table[512] = {
    [1]   = (syscall_t) sys_exit,
    [2]   = (syscall_t) sys_fork,
    [3]   = (syscall_t) sys_read,
    [4]   = (syscall_t) sys_write,
    [5]   = (syscall_t) sys_open,
    [6]   = (syscall_t) sys_close,
    [7]   = (syscall_t) sys_waitpid,
    [11]  = (syscall_t) sys_execve,
    [12]  = (syscall_t) sys_chdir,
    [20]  = (syscall_t) sys_getpid,
    [45]  = (syscall_t) sys_brk,
    [64]  = (syscall_t) sys_getppid,
    [90]  = (syscall_t) sys_mmap2,
    [91]  = (syscall_t) sys_munmap,
    [114] = (syscall_t) sys_wait4,
    [120] = (syscall_t) sys_clone,
    [125] = (syscall_t) sys_mprotect,
    [183] = (syscall_t) sys_getcwd,
    [192] = (syscall_t) sys_mmap2,
    [195] = (syscall_t) sys_stat64,
    [196] = (syscall_t) sys_lstat64,
    [197] = (syscall_t) sys_fstat64,

    [281] = (syscall_t) sys_socket,
    [282] = (syscall_t) sys_bind,
    [283] = (syscall_t) sys_connect,
    [284] = (syscall_t) sys_listen,
    [285] = (syscall_t) sys_accept,
    [289] = (syscall_t) sys_send,
    [291] = (syscall_t) sys_recv,
    [37]  = (syscall_t) sys_kill,
    [39]  = (syscall_t) sys_mkdir,
    [40]  = (syscall_t) sys_rmdir,
    [9]   = (syscall_t) sys_link,
    [10]  = (syscall_t) sys_unlink,
};

long handle_syscall(int num, long arg1, long arg2, long arg3, 
                   long arg4, long arg5, long arg6) {
    if (num < 0 || num >= 512 || syscall_table[num] == NULL) {
        fprintf(stderr, "termi: unsupported syscall %d\n", num);
        return _ENOSYS;
    }
    
    syscall_t handler = syscall_table[num];
    return handler(arg1, arg2, arg3, arg4, arg5, arg6);
}
