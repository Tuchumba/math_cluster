#define _POSIX_C_SOURCE 200809L
#include "distr.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

int run_worker(const char *host, const char *port, int max_cores, int max_time_sec) {
    int fd = net_connect(host, port, 5);
    if (fd < 0) { perror("connect"); return 2; }
    char hello[128];
    snprintf(hello, sizeof(hello), "HELLO cores=%d timeout=%d", max_cores, max_time_sec);
    if (net_send_line(fd, hello) < 0) { perror("send"); close(fd); return 2; }

    char buf[256];
    if (net_recv_line(fd, buf, sizeof(buf), max_time_sec) < 0) { fprintf(stderr, "no task\n"); close(fd); return 2; }
    if (strncmp(buf, "SHUTDOWN", 8) == 0) { close(fd); return 0; }
    double a=0,b=0; long n=0; int id=0,chunks=1;
    if (sscanf(buf, "TASK a=%lf b=%lf n=%ld id=%d chunks=%d", &a,&b,&n,&id,&chunks) < 4) {
        fprintf(stderr, "bad TASK: %s\n", buf); close(fd); return 2;
    }
    int threads = max_cores;
    if (chunks > threads) threads = chunks;

    int timed_out = 0;
    double val = integrate_trapz(a,b,n, threads, max_time_sec, &timed_out);
    if (timed_out) {
        fprintf(stderr, "[worker] timed out\n");
        close(fd); return 3;
    }
    char line[256];
    snprintf(line, sizeof(line), "RESULT id=%d value=%.17g", id, val);
    if (net_send_line(fd, line) < 0) { perror("send result"); close(fd); return 2; }

    if (net_recv_line(fd, buf, sizeof(buf), 5) == 0 && strncmp(buf,"SHUTDOWN",8)==0) {
        close(fd); return 0;
    }
    close(fd);
    return 0;
}
