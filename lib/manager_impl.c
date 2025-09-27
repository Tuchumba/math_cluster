#define _POSIX_C_SOURCE 200809L
#include "distr.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

typedef struct {
    int sock;
    int cores;
    int timeout;
    int alive;
} worker_info_t;

static volatile sig_atomic_t stop_flag = 0;

static void handle_sigint(int sig) { (void)sig; stop_flag = 1; }

static void broadcast_shutdown(worker_info_t *ws, int n) {
    for (int i=0;i<n;i++) if (ws[i].alive) net_send_line(ws[i].sock, "SHUTDOWN");
}

int run_manager(int required_workers, int max_time_sec,
                const char *host, const char *port,
                double a, double b, long n) {
    int listen_fd = net_listen(host, port);
    if (listen_fd < 0) { perror("listen"); return 2; }

    signal(SIGINT, handle_sigint);
    fprintf(stderr, "[manager] listening on %s:%s, need %d workers\n", host, port, required_workers);

    worker_info_t *ws = calloc(required_workers, sizeof(worker_info_t));
    int connected = 0;
    uint64_t t0 = now_ms();

    while (connected < required_workers) {
        if (stop_flag) { fprintf(stderr, "Interrupted\n"); goto fail; }
        if ((now_ms() - t0) / 1000 > (uint64_t)max_time_sec) { fprintf(stderr, "Timeout waiting workers\n"); goto fail; }
        int fd = net_accept(listen_fd, 2);
        if (fd < 0) continue;
        char buf[256];
        if (net_recv_line(fd, buf, sizeof(buf), 5) < 0) { close(fd); continue; }
        int cores=1, timeout=10;
        if (sscanf(buf, "HELLO cores=%d timeout=%d", &cores, &timeout) != 2) { close(fd); continue; }
        ws[connected].sock = fd;
        ws[connected].cores = cores;
        ws[connected].timeout = timeout;
        ws[connected].alive = 1;
        connected++;
        fprintf(stderr, "[manager] worker %d joined (cores=%d, timeout=%d)\n", connected, cores, timeout);
    }

    uint64_t t1 = now_ms();

    int total_cores = 0;
    for (int i=0;i<required_workers;i++) total_cores += ws[i].cores;
    double start = a;
    for (int i=0;i<required_workers;i++) {
        long m = n / total_cores;
        double frac = (double)ws[i].cores / (double)total_cores;
        double end = start + (b - a) * frac;
        char line[256];
        snprintf(line, sizeof(line), "TASK a=%.17g b=%.17g n=%ld id=%d chunks=%d",
                 start, end, m * ws[i].cores, i, ws[i].cores);
        if (net_send_line(ws[i].sock, line) < 0) { fprintf(stderr, "send fail\n"); goto fail_broadcast; }
        start = end;
    }

    double total = 0.0;
    for (int i=0;i<required_workers;i++) {
        char buf[256];
        if (net_recv_line(ws[i].sock, buf, sizeof(buf), max_time_sec) < 0) {
            fprintf(stderr, "worker %d failed or timeout\n", i);
            goto fail_broadcast;
        }
        int id=0; double val=0.0;
        if (sscanf(buf, "RESULT id=%d value=%lf", &id, &val) != 2) {
            fprintf(stderr, "bad result: %s\n", buf);
            goto fail_broadcast;
        }
        total += val;
    }
    uint64_t t2 = now_ms();
    double secs = (t2 - t1)/1000.0;
    printf("INTEGRAL=%.12f\n", total);
    printf("TOTAL_TIME_SEC=%.6f\n", secs);
    printf("TOTAL_CORES=%d\n", total_cores);
    broadcast_shutdown(ws, required_workers);
    for (int i=0;i<required_workers;i++) close(ws[i].sock);
    close(listen_fd);
    free(ws);
    return 0;

fail_broadcast:
    broadcast_shutdown(ws, required_workers);
fail:
    for (int i=0;i<required_workers;i++) if (ws[i].sock>0) close(ws[i].sock);
    close(listen_fd);
    free(ws);
    return 3;
}
