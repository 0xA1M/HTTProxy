#ifndef PROXY_H
#define PROXY_H

/* Standard Libraries */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* POSIX Multi-Threading Library */
#include <pthread.h>

/* Logging Library */
#include "clog.h"

void *proxy(void *arg);

#endif /* PROXY_H */
