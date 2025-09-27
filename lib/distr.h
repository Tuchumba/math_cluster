#ifndef DISTR_H
#define DISTR_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int sockfd;
} net_conn_t;

typedef struct {
    int max_cores;        
    int max_time_sec;     
} worker_cfg_t;

typedef struct {
    int required_workers; 
    int max_time_sec;     
} manager_cfg_t;

typedef struct {
    double a;
    double b;
    long n;
    int    chunks;        
    int    id;            
} task_t;

typedef struct {
    int id;
    double value;
} task_result_t;

int net_listen(const char *host, const char *port);            
int net_accept(int listen_fd, int timeout_sec);                 
int net_connect(const char *host, const char *port, int timeout_sec); 
int net_send_line(int sockfd, const char *line);                
int net_recv_line(int sockfd, char *buf, size_t bufsz, int timeout_sec); 

double integrate_trapz(double a, double b, long n, int threads, int max_time_sec, int *timed_out);

int run_manager(int required_workers, int max_time_sec, const char *host, const char *port, double a, double b, long n);

int run_worker(const char *host, const char *port, int max_cores, int max_time_sec);

uint64_t now_ms(void);
int set_socket_timeout(int fd, int sec);

#ifdef __cplusplus
}
#endif

#endif 
