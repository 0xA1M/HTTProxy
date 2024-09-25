#ifndef HANDLER_H
#define HANDLER_H

/* Standard Library */
#include <stdbool.h>

/* POSIX Multiplexing Library */
#include <sys/poll.h>

/* Parser */
#include "parser.h"

#define TIMEOUT -1

int client_handler(const int client_fd, int *server_fd, struct pollfd fds[2],
                   Request *req, bool *is_TLS);

void *handler(void *arg);

#endif /* HANDLER_H */
