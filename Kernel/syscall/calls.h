#ifndef TERMI_SYSCALL_CALLS_H
#define TERMI_SYSCALL_CALLS_H

#include "../include/types.h"

typedef long (*syscall_t)(long, long, long, long, long, long);

long handle_syscall(int num, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

dword_t syscall_stub(void);
dword_t syscall_silent_stub(void);
dword_t syscall_success_stub(void);

dword_t sys_exit(dword_t status);
dword_t sys_fork(void);
dword_t sys_read(fd_t fd_no, addr_t buf_addr, dword_t size);
dword_t sys_write(fd_t fd_no, addr_t buf_addr, dword_t size);
dword_t sys_open(addr_t path_addr, dword_t flags, mode_t_ mode);
dword_t sys_close(fd_t fd);
dword_t sys_waitpid(pid_t_ pid, addr_t status_addr, dword_t options);
dword_t sys_wait4(pid_t_ pid, addr_t status_addr, dword_t options, addr_t rusage_addr);
dword_t sys_execve(addr_t file, addr_t argv, addr_t envp);
dword_t sys_chdir(addr_t path_addr);
pid_t_ sys_getpid(void);
pid_t_ sys_getppid(void);
addr_t sys_brk(addr_t new_brk);
addr_t sys_mmap2(addr_t addr, dword_t len, dword_t prot, dword_t flags, fd_t fd_no, dword_t offset);
int_t sys_munmap(addr_t addr, uint_t len);
int_t sys_mprotect(addr_t addr, uint_t len, int_t prot);
dword_t sys_kill(pid_t_ pid, int_t sig);
dword_t sys_clone(dword_t flags, addr_t stack, addr_t ptid, addr_t tls, addr_t ctid);
dword_t sys_stat64(addr_t path_addr, addr_t statbuf_addr);
dword_t sys_lstat64(addr_t path_addr, addr_t statbuf_addr);
dword_t sys_fstat64(fd_t fd_no, addr_t statbuf_addr);
dword_t sys_getcwd(addr_t buf_addr, dword_t size);
dword_t sys_mkdir(addr_t path_addr, mode_t_ mode);
dword_t sys_rmdir(addr_t path_addr);
dword_t sys_link(addr_t src_addr, addr_t dst_addr);
dword_t sys_unlink(addr_t path_addr);
dword_t sys_socket(int_t domain, int_t type, int_t protocol);
dword_t sys_bind(fd_t sockfd, addr_t addr, uint_t addrlen);
dword_t sys_connect(fd_t sockfd, addr_t addr, uint_t addrlen);
dword_t sys_listen(fd_t sockfd, int_t backlog);
dword_t sys_accept(fd_t sockfd, addr_t addr, addr_t addrlen);
dword_t sys_send(fd_t sockfd, addr_t buf, dword_t len, int_t flags);
dword_t sys_recv(fd_t sockfd, addr_t buf, dword_t len, int_t flags);

int must_check user_read(addr_t addr, void *buf, size_t count);
int must_check user_write(addr_t addr, const void *buf, size_t count);
int must_check user_read_string(addr_t addr, char *buf, size_t max);
int must_check user_write_string(addr_t addr, const char *buf);

#define user_get(addr, var) user_read(addr, &(var), sizeof(var))
#define user_put(addr, var) user_write(addr, &(var), sizeof(var))

#endif
