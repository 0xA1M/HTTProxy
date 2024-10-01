#ifndef HANDLER_H
#define HANDLER_H

/* Standard Library */
#include <stdbool.h>

/* POSIX Multiplexing Library */
#include <sys/poll.h>

#define TIMEOUT -1

void *handler(void *arg);

#endif /* HANDLER_H */
