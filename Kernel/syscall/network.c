#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include "calls.h"

#define AF_UNIX_ 1
#define AF_INET_ 2
#define AF_INET6_ 10

#define SOCK_STREAM_ 1
#define SOCK_DGRAM_ 2
#define SOCK_RAW_ 3
#define SOCK_NONBLOCK_ 0x800
#define SOCK_CLOEXEC_ 0x80000

static int translate_domain(int_t linux_domain) {
    switch (linux_domain) {
        case AF_UNIX_: return AF_UNIX;
        case AF_INET_: return AF_INET;
        case AF_INET6_: return AF_INET6;
        default: return linux_domain;
    }
}

static int translate_socket_type(int_t linux_type) {
    int darwin_type = 0;
    int base_type = linux_type & 0xff;
    
    switch (base_type) {
        case SOCK_STREAM_: darwin_type = SOCK_STREAM; break;
        case SOCK_DGRAM_: darwin_type = SOCK_DGRAM; break;
        case SOCK_RAW_: darwin_type = SOCK_RAW; break;
        default: darwin_type = base_type; break;
    }
    
    return darwin_type;
}

dword_t sys_socket(int_t domain, int_t type, int_t protocol) {
    int darwin_domain = translate_domain(domain);
    int darwin_type = translate_socket_type(type);
    
    int fd = socket(darwin_domain, darwin_type, protocol);
    
    if (fd < 0) {
        return -errno;
    }
    
    return fd;
}

dword_t sys_bind(fd_t sockfd, addr_t addr, uint_t addrlen) {
    struct sockaddr_storage sa;
    
    if (addrlen > sizeof(sa)) {
        return _EINVAL;
    }
    
    if (user_read(addr, &sa, addrlen) < 0) {
        return _EFAULT;
    }
    
    if (bind(sockfd, (struct sockaddr *)&sa, addrlen) < 0) {
        return -errno;
    }
    
    return 0;
}

dword_t sys_connect(fd_t sockfd, addr_t addr, uint_t addrlen) {
    struct sockaddr_storage sa;
    
    if (addrlen > sizeof(sa)) {
        return _EINVAL;
    }
    
    if (user_read(addr, &sa, addrlen) < 0) {
        return _EFAULT;
    }
    
    if (connect(sockfd, (struct sockaddr *)&sa, addrlen) < 0) {
        return -errno;
    }
    
    return 0;
}

dword_t sys_listen(fd_t sockfd, int_t backlog) {
    if (listen(sockfd, backlog) < 0) {
        return -errno;
    }
    return 0;
}

dword_t sys_accept(fd_t sockfd, addr_t addr, addr_t addrlen) {
    struct sockaddr_storage sa;
    socklen_t len = sizeof(sa);
    
    int fd = accept(sockfd, (struct sockaddr *)&sa, &len);
    
    if (fd < 0) {
        return -errno;
    }
    
    if (addr != 0) {
        uint_t user_len;
        if (user_get(addrlen, user_len) < 0) {
            return _EFAULT;
        }
        
        uint_t copy_len = len < user_len ? len : user_len;
        if (user_write(addr, &sa, copy_len) < 0) {
            return _EFAULT;
        }
        
        if (user_put(addrlen, len) < 0) {
            return _EFAULT;
        }
    }
    
    return fd;
}

dword_t sys_send(fd_t sockfd, addr_t buf, dword_t len, int_t flags) {
    char buffer[8192];
    dword_t total_sent = 0;
    
    while (len > 0) {
        size_t chunk = (size_t)len > sizeof(buffer) ? sizeof(buffer) : (size_t)len;
        
        if (user_read(buf + total_sent, buffer, chunk) < 0) {
            return total_sent > 0 ? total_sent : _EFAULT;
        }
        
        ssize_t sent = send(sockfd, buffer, chunk, flags);
        
        if (sent < 0) {
            return total_sent > 0 ? total_sent : -errno;
        }
        
        total_sent += sent;
        len -= sent;
        
        if (sent < (ssize_t)chunk) {
            break;
        }
    }
    
    return total_sent;
}

dword_t sys_recv(fd_t sockfd, addr_t buf, dword_t len, int_t flags) {
    char buffer[8192];
    dword_t total_recv = 0;
    
    while (len > 0) {
        size_t chunk = (size_t)len > sizeof(buffer) ? sizeof(buffer) : (size_t)len;
        
        ssize_t received = recv(sockfd, buffer, chunk, flags);
        
        if (received < 0) {
            return total_recv > 0 ? total_recv : -errno;
        }
        
        if (received == 0) {
            break;
        }
        
        if (user_write(buf + total_recv, buffer, received) < 0) {
            return total_recv > 0 ? total_recv : _EFAULT;
        }
        
        total_recv += received;
        len -= received;
        
        if (received < (ssize_t)chunk) {
            break;
        }
    }
    
    return total_recv;
}
