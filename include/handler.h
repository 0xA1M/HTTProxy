#ifndef HANDLER_H
#define HANDLER_H

/* Standard Library */
#include <stdbool.h>

/* POSIX Multiplexing Library */
#include <sys/poll.h>

/* Parser */
#include "parser.h"

/* Data Structures */
typedef struct ClientInfo {
  int client_fd;
  int server_fd;

  Request *req;
} ClientInfo;

#define TIMEOUT -1

void *handler(void *arg);

int client_handler(const int client_fd, int *server_fd, struct pollfd fds[2],
                   Request *req, bool *is_TLS);

#endif /* HANDLER_H */
