#ifndef HANDLER_H
#define HANDLER_H

/* Standard Library */
#include <stdbool.h>

/* POSIX Multiplexing Library */
#include <sys/poll.h>

/* Parser */
#include "parser.h"

/* Data Structures */
typedef struct ConnInfo {
  struct pollfd fds[2];

  Request *req;
  Response *res;
} ConnInfo;

#define TIMEOUT 120000 // 120 seconds

void *handler(void *arg);

int client_handler(struct pollfd *fds, Request *req, bool *is_TLS);

int server_handler(const struct pollfd *fds, Response *res, bool *is_TLS);

#endif /* HANDLER_H */
