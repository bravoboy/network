#include "network.h"
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#define QUEUE 10240

int network_set_block(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return flags;
    }

    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

int network_set_noblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return flags;
    }

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int network_set_reuse(int fd) {
    int reuse = 1;
    socklen_t len = sizeof(reuse);

    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, len);
}

int network_set_tcpnodelay(int fd) {
    int nodelay = 1;
    socklen_t len = sizeof(nodelay);

    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, len);
}

static int network_get_socket_error(int fd) {
    int err = 0;
    socklen_t len = sizeof(err);

    int status = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
    if (status != 0) {
        return -1;
    }
    if (err != 0) {
        errno = err;
        return -1;
    }
    return 0;
}

int network_bind_listen(int port, int reuse) {
    int fd = socket(AF_INET,SOCK_STREAM, 0);

    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(port);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (reuse) {
        network_set_reuse(fd);
    }

    if (bind(fd,(struct sockaddr *)&server_sockaddr,sizeof(server_sockaddr)) == -1) {
        close(fd);
        return -1;
    }
    if (listen(fd,QUEUE) == -1) {
        close(fd);
        return -1;
    }
    return fd;
}

int network_connet(const char *address, int port, int timeout_ms) {
    struct sockaddr_in    servaddr;
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }   
    memset((void*)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, address, &servaddr.sin_addr) <= 0) {
        close(sockfd);
        return -1;
    }   
    if (timeout_ms > 0) {
        if (network_set_noblock(sockfd) != 0) {
            close(sockfd);
            return -1;
        }   
    }   
    if (connect(sockfd,(struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        if (timeout_ms > 0 && (errno == EINPROGRESS || errno == EINTR)) {
            if (wait_ready(sockfd,timeout_ms) != 0) {
                close(sockfd);
                return -1;
            }   
            if (network_set_noblock(sockfd) != 0) {
                close(sockfd);
                return -1;
            } 
            return sockfd;
        } else {
            close(sockfd);
            return -1;
        }   
    } else {
        //Connect was successfull immediately.
        if (timeout_ms > 0) {
            if (network_set_noblock(sockfd) != 0) {
                close(sockfd);
                return -1;
            }   
        }   
        return sockfd;
    }   
}

static int wait_ready(int fd, int msec) {
    struct pollfd   wfd[1];

    wfd[0].fd     = fd;
    wfd[0].events = POLLOUT;

    int res;

    if ((res = poll(wfd, 1, msec)) == -1) {
        return -1;
    } else if (res == 0) {
        return -1;
    }

    if (network_get_socket_error(fd) != 0) {
        return -1;
    }
    return 0;
}

