#include "lib/distr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int run_worker(const char *host, const char *port, int max_cores, int max_time_sec);

static void usage(const char *p) {
    fprintf(stderr, "Usage: %s --host <host> --port <port> [--cores N] [--timeout S]\n", p);
}

int main(int argc, char **argv) {
    const char *host = "127.0.0.1";
    const char *port = "5555";
    int cores = 1;
    int timeout = 30;
    for (int i=1;i<argc;i++) {
        if (!strcmp(argv[i],"--host") && i+1<argc) host = argv[++i];
        else if (!strcmp(argv[i],"--port") && i+1<argc) port = argv[++i];
        else if (!strcmp(argv[i],"--cores") && i+1<argc) cores = atoi(argv[++i]);
        else if (!strcmp(argv[i],"--timeout") && i+1<argc) timeout = atoi(argv[++i]);
    }
    if (!host || !port) { usage(argv[0]); return 1; }
    return run_worker(host, port, cores, timeout);
}
