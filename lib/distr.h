#ifndef DISTR_H
#define DISTR_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// === Общие параметры/структуры ===
typedef struct {
    int sockfd;
} net_conn_t;

// Конфиг рабочего узла
typedef struct {
    int max_cores;        // ограничение по потокам
    int max_time_sec;     // максимальное допустимое время вычислений
} worker_cfg_t;

// Конфиг управляющего узла
typedef struct {
    int required_workers; // сколько рабочих нужно
    int max_time_sec;     // общий максимум на вычисление
} manager_cfg_t;

// Задача интегрирования
typedef struct {
    double a;
    double b;
    long n;
    int    chunks;        // сколько чанков внутри подзадачи
    int    id;            // идентификатор подзадачи
} task_t;

// Результат подзадачи
typedef struct {
    int id;
    double value;
} task_result_t;

// === API сети ===
int net_listen(const char *host, const char *port);            // возвращает listen fd
int net_accept(int listen_fd, int timeout_sec);                 // accept с таймаутом
int net_connect(const char *host, const char *port, int timeout_sec); // connect с таймаутом
int net_send_line(int sockfd, const char *line);                // отправка строки \n-terminated
int net_recv_line(int sockfd, char *buf, size_t bufsz, int timeout_sec); // чтение строки

// === Интеграл ===
double integrate_trapz(double a, double b, long n, int threads, int max_time_sec, int *timed_out);
// === Хэлперы ===
uint64_t now_ms(void);
int set_socket_timeout(int fd, int sec);

#ifdef __cplusplus
}
#endif

#endif // DISTR_H
