#include "lib/distr.h"
#include <stdio.h>
#include <stdlib.h>

int run_worker(const char *host, const char *port, int max_cores, int max_time_sec);

int main(void) {
    // smoke test: try to connect to non-existing manager quickly
    int code = run_worker("127.0.0.1", "5560", 1, 1);
    printf("worker_exit_code=%d\n", code);
    return 0;
}
