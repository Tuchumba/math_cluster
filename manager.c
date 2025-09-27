#include "lib/distr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int run_manager(int required_workers, int max_time_sec,
                const char *host, const char *port,
                double a, double b, long n);

static void usage(const char *p) {
    fprintf(stderr, "Usage: %s <workers> <host> <port> --a <A> --b <B> --n <N> [--timeout <sec>]\n", p);
}

int main(int argc, char **argv) {
    if (argc < 8) { usage(argv[0]); return 1; }
    int workers = atoi(argv[1]);
    const char *host = argv[2];
    const char *port = argv[3];
    double a=0,b=1,n=0;
    int timeout=30;
    for (int i=4;i<argc;i++) {
        if (!strcmp(argv[i],"--a") && i+1<argc) a = atof(argv[++i]);
        else if (!strcmp(argv[i],"--b") && i+1<argc) b = atof(argv[++i]);
        else if (!strcmp(argv[i],"--n") && i+1<argc) n = atoi(argv[++i]);
        else if (!strcmp(argv[i],"--timeout") && i+1<argc) timeout = atoi(argv[++i]);
    }
    return run_manager(workers, timeout, host, port, a, b, n);
}
