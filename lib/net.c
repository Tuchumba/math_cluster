#define _POSIX_C_SOURCE 200809L
#include "distr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>

static int setsockopts_common(int fd) {
    int one = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) return -1;
#ifdef SO_REUSEPORT
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
#endif
    return 0;
}

int set_socket_timeout(int fd, int sec) {
    struct timeval tv = { .tv_sec = sec, .tv_usec = 0 };
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) return -1;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) return -1;
    return 0;
}

int net_listen(const char *host, const char *port) {
    struct addrinfo hints, *res, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(host, port, &hints, &res) != 0) return -1;
    int listen_fd = -1;
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        listen_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listen_fd < 0) continue;
        setsockopts_common(listen_fd);
        if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(listen_fd); listen_fd = -1;
    }
    freeaddrinfo(res);
    if (listen_fd < 0) return -1;
    if (listen(listen_fd, 64) < 0) { close(listen_fd); return -1; }
    return listen_fd;
}

int net_accept(int listen_fd, int timeout_sec) {
    set_socket_timeout(listen_fd, timeout_sec);
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listen_fd, (struct sockaddr*)&ss, &slen);
    return fd;
}

int net_connect(const char *host, const char *port, int timeout_sec) {
    struct addrinfo hints, *res, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) return -1;
    int fd = -1;
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) continue;
        set_socket_timeout(fd, timeout_sec);
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(fd); fd = -1;
    }
    freeaddrinfo(res);
    return fd;
}

int net_send_line(int sockfd, const char *line) {
    size_t n = strlen(line);
    if (send(sockfd, line, n, 0) != (ssize_t)n) return -1;
    if (send(sockfd, "\n", 1, 0) != 1) return -1;
    return 0;
}

int net_recv_line(int sockfd, char *buf, size_t bufsz, int timeout_sec) {
    set_socket_timeout(sockfd, timeout_sec);
    size_t i = 0;
    while (i + 1 < bufsz) {
        char c;
        ssize_t r = recv(sockfd, &c, 1, 0);
        if (r == 0) return -1; // EOF
        if (r < 0) return -1;
        if (c == '\n') break;
        buf[i++] = c;
    }
    buf[i] = '\0';
    return 0;
}

uint64_t now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000ULL + tv.tv_usec / 1000ULL;
}
