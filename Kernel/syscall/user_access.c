#include <string.h>
#include <stdint.h>
#include "calls.h"

extern void *emulator_memory_base;
extern size_t emulator_memory_size;

static inline void *guest_to_host(addr_t addr) {
    if (emulator_memory_base == NULL) {
        return NULL;
    }
    
    if (addr >= emulator_memory_size) {
        return NULL;
    }
    
    return (char *)emulator_memory_base + addr;
}

int must_check user_read(addr_t addr, void *buf, size_t count) {
    void *host_addr = guest_to_host(addr);
    if (host_addr == NULL) {
        return -_EFAULT;
    }
    
    if (addr + count > emulator_memory_size) {
        return -_EFAULT;
    }
    
    memcpy(buf, host_addr, count);
    return 0;
}

int must_check user_write(addr_t addr, const void *buf, size_t count) {
    void *host_addr = guest_to_host(addr);
    if (host_addr == NULL) {
        return -_EFAULT;
    }
    
    if (addr + count > emulator_memory_size) {
        return -_EFAULT;
    }
    
    memcpy(host_addr, buf, count);
    return 0;
}

int must_check user_read_string(addr_t addr, char *buf, size_t max) {
    void *host_addr = guest_to_host(addr);
    if (host_addr == NULL) {
        return -_EFAULT;
    }
    
    size_t i;
    for (i = 0; i < max - 1; i++) {
        if (addr + i >= emulator_memory_size) {
            return -_EFAULT;
        }
        
        char c = *((char *)host_addr + i);
        buf[i] = c;
        
        if (c == '\0') {
            return 0;
        }
    }
    
    buf[max - 1] = '\0';
    return -_EFAULT;
}

int must_check user_write_string(addr_t addr, const char *buf) {
    size_t len = strlen(buf);
    return user_write(addr, buf, len + 1);
}
