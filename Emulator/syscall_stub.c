#include <stdint.h>

long handle_syscall(int num, uint64_t arg1, uint64_t arg2, uint64_t arg3,
                   uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    (void)num; (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return -1;
}
