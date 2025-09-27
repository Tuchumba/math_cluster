#define _POSIX_C_SOURCE 200809L
#include "distr.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>

typedef struct {
    pthread_mutex_t mtx;
    double a, b;
    long n;
    int   max_time_sec;
    atomic_int timed_out;
    atomic_long iter_done;
    double partial;
} worker_ctx_t;

// Целевая функция (можно заменить)
static inline double f(double x) {
    return 4.0 / (1.0 + x*x); // интеграл [0,1] -> π
}

typedef struct {
    pthread_mutex_t mtx;
    worker_ctx_t *ctx;
    double start, end;
    long n;
} thread_args_t;

static void* thread_run(void *arg_) {
    thread_args_t *arg = (thread_args_t*)arg_;
    worker_ctx_t *ctx = arg->ctx;
    double a = arg->start, b = arg->end;
    long n = arg->n;
    double h = (b - a) / (double)n;
    double sum = 0.0;
    uint64_t t0 = now_ms();
    for (long i = 0; i < n; ++i) {
        if (ctx->max_time_sec > 0 && (now_ms() - t0) > (uint64_t)ctx->max_time_sec * 1000ULL) {
            atomic_store(&ctx->timed_out, 1);
            break;
        }
        double x1 = a + i * h;
        double x2 = x1 + h;
        sum += 0.5 * (f(x1) + f(x2)) * h;
        atomic_fetch_add(&ctx->iter_done, 1);
    }
    pthread_mutex_lock(&ctx->mtx);
    ctx->partial += sum;
    pthread_mutex_unlock(&ctx->mtx);
    return NULL;
}

double integrate_trapz(double a, double b, long n, int threads, int max_time_sec, int *timed_out) {
    if (threads < 1) threads = 1;

    worker_ctx_t ctx = { .a=a, .b=b, .n=n, .max_time_sec=max_time_sec };
    pthread_mutex_init(&ctx.mtx, NULL);
    atomic_init(&ctx.timed_out, 0);
    atomic_init(&ctx.iter_done, 0);
    ctx.partial = 0.0;

    pthread_t *ths = calloc(threads, sizeof(pthread_t));
    thread_args_t *args = calloc(threads, sizeof(thread_args_t));
    long chunk = n / threads;
    double h = (b - a) / (double)threads;
    for (int t=0; t<threads; ++t) {
        args[t].ctx = &ctx;
        args[t].n = chunk;
        args[t].start = ctx.a + t * h;
        args[t].end   = ctx.a + (t + 1) * h;
        pthread_create(&ths[t], NULL, thread_run, &args[t]);
    }
    for (int t=0; t<threads; ++t) pthread_join(ths[t], NULL);

    if (timed_out) *timed_out = atomic_load(&ctx.timed_out);
    double res = ctx.partial;
    free(ths); free(args);
    pthread_mutex_destroy(&ctx.mtx);
    return res;
}
