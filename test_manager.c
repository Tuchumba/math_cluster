#include "lib/distr.h"
#include <stdio.h>
#include <stdlib.h>

int run_manager(int required_workers, int max_time_sec,
                const char *host, const char *port,
                double a, double b, long n);

int main(void) {
    // smoke test: start manager that expects 1 worker and exits on timeout (no worker started here)
    int code = run_manager(1, 2, "127.0.0.1", "5559", 0.0, 1.0, 10000);
    printf("manager_exit_code=%d\n", code);
    return 0;
}
