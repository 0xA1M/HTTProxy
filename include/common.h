#ifndef COMMON_H
#define COMMON_H

/* Standard Libraries */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* POSIX Networking Library */
#include <netdb.h>

/* POSIX Multi-Threading Library */
#include <pthread.h>

/* Logging Library */
#include "clog.h"

/* Thread Pool */
#define MAX_THREADS 64 // Handle 64 simultaneous connections

extern pthread_t thread_pool[MAX_THREADS];
extern int thread_count;

#define MAX_HTTP_LEN 8192
#define MAX_PORT_LEN 6 // Max length of the port number

/* Utility Functions */
int find_empty_slot(void);

void remove_thread(pthread_t tid);

int forward(const int dest_fd, const unsigned char *buffer, const ssize_t len);

#endif /* COMMON_H */
