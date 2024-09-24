#ifndef HANDLER_H
#define HANDLER_H

/* POSIX Multiplexing Library */
#include "parser.h"
#include <sys/poll.h>

#define TIMEOUT -1

int client_handler(const int client_fd, int *server_fd, struct pollfd fds[2],
                   Request *req);

void *handler(void *arg);

#endif /* HANDLER_H */
