#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "calls.h"

extern char **environ;

dword_t sys_exit(dword_t status) {
    exit(status & 0xff);
    __builtin_unreachable();
}

pid_t_ sys_getpid(void) {
    return getpid();
}

pid_t_ sys_getppid(void) {
    return getppid();
}

dword_t sys_fork(void) {
    pid_t pid;
    posix_spawnattr_t attr;
    
    posix_spawnattr_init(&attr);
    posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETEXEC);
    
    pid = fork();
    if (pid < 0) {
        return -errno;
    }
    
    posix_spawnattr_destroy(&attr);
    return pid;
}

dword_t sys_execve(addr_t file_addr, addr_t argv_addr, addr_t envp_addr) {
    char filename[4096];
    char *argv[256];
    char *envp[256];
    int argc = 0, envc = 0;
    
    if (user_read_string(file_addr, filename, sizeof(filename)) < 0) {
        return _EFAULT;
    }
    
    addr_t arg_ptr;
    addr_t current_argv = argv_addr;
    while (argc < 255) {
        if (user_get(current_argv, arg_ptr) < 0) {
            return _EFAULT;
        }
        if (arg_ptr == 0) break;
        
        argv[argc] = malloc(4096);
        if (user_read_string(arg_ptr, argv[argc], 4096) < 0) {
            for (int i = 0; i < argc; i++) free(argv[i]);
            return _EFAULT;
        }
        argc++;
        current_argv += sizeof(addr_t);
    }
    argv[argc] = NULL;
    
    if (envp_addr != 0) {
        addr_t current_envp = envp_addr;
        while (envc < 255) {
            if (user_get(current_envp, arg_ptr) < 0) {
                for (int i = 0; i < argc; i++) free(argv[i]);
                return _EFAULT;
            }
            if (arg_ptr == 0) break;
            
            envp[envc] = malloc(4096);
            if (user_read_string(arg_ptr, envp[envc], 4096) < 0) {
                for (int i = 0; i < argc; i++) free(argv[i]);
                for (int i = 0; i < envc; i++) free(envp[i]);
                return _EFAULT;
            }
            envc++;
            current_envp += sizeof(addr_t);
        }
        envp[envc] = NULL;
        execve(filename, argv, envp);
    } else {
        execve(filename, argv, environ);
    }
    
    int err = errno;
    for (int i = 0; i < argc; i++) free(argv[i]);
    for (int i = 0; i < envc; i++) free(envp[i]);
    return -err;
}

dword_t sys_waitpid(pid_t_ pid, addr_t status_addr, dword_t options) {
    int status;
    pid_t result = waitpid(pid, &status, options);
    
    if (result < 0) {
        return -errno;
    }
    
    if (status_addr != 0) {
        if (user_put(status_addr, status) < 0) {
            return _EFAULT;
        }
    }
    
    return result;
}

dword_t sys_wait4(pid_t_ pid, addr_t status_addr, dword_t options, addr_t rusage_addr) {
    int status;
    struct rusage rusage;
    
    pid_t result = wait4(pid, &status, options, rusage_addr ? &rusage : NULL);
    
    if (result < 0) {
        return -errno;
    }
    
    if (status_addr != 0) {
        if (user_put(status_addr, status) < 0) {
            return _EFAULT;
        }
    }
    
    return result;
}

dword_t sys_clone(dword_t flags, addr_t stack, addr_t ptid, addr_t tls, addr_t ctid) {
    (void)stack;
    (void)ptid;
    (void)tls;
    (void)ctid;
    
    if (flags & 0x00010000) {
        return _ENOSYS;
    }
    
    return sys_fork();
}

dword_t sys_kill(pid_t_ pid, int_t sig) {
    if (kill(pid, sig) < 0) {
        return -errno;
    }
    return 0;
}
