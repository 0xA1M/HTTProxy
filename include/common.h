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

/* Parser */
#include "parser.h"

/* Thread Pool */
#define MAX_THREADS 64 // Handle 64 simultaneous connections

extern pthread_t thread_pool[MAX_THREADS];
extern int thread_count;
extern pthread_mutex_t lock;

#define MAX_HTTP_LEN 8192    // Max length of the http request/response
#define MAX_HOSTNAME_LEN 256 // Max length of the hostname
#define MAX_PORT_LEN 6       // Max length of the port number

/* Utility Functions */
int find_empty_slot(void);

void remove_thread(const pthread_t tid);

int forward(const int dest_fd, const unsigned char *buffer, const long len);

void print_req(const Request *req);

void print_res(const Response *res);

void free_req(Request *req);

void free_res(Response *res);

char *get_header_value(const char *target, const Header *headers,
                       const size_t headers_count);

int get_chunk_size(const unsigned char *chunk, const size_t chunk_len);

void print_hex(const unsigned char *buffer, const size_t buffer_len);

#endif /* COMMON_H */
