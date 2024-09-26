#include <arpa/inet.h>

#include "common.h"
#include "handler.h"
#include "proxy.h"

#define BACKLOG 16 // Max members in listening queue

static int init_proxy(const char *port) {
  struct addrinfo hints, *res, *p;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int status = getaddrinfo(NULL, port, &hints, &res);
  if (status != 0) {
    LOG(ERR, gai_strerror(status), "getaddrinfo failed");
    freeaddrinfo(res);
    return -1;
  }

  int proxy_fd = -1, opt = 1;
  for (p = res; p != NULL; p = p->ai_next) {
    proxy_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (proxy_fd == -1) {
      LOG(WARN, NULL, "Failed to create proxy server socket (retrying...)");
      continue;
    }

    if (setsockopt(proxy_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt) ==
        -1) {
      LOG(ERR, NULL, "Failed to set socket options to allow address reuse");
      freeaddrinfo(res);
      close(proxy_fd);
      return -1;
    }

    if (bind(proxy_fd, res->ai_addr, res->ai_addrlen) == -1) {
      LOG(WARN, NULL,
          "Failed to bind proxy server socket to address (retry...)");
      close(proxy_fd);
      continue;
    }

    break;
  }

  freeaddrinfo(res);
  if (p == NULL) {
    LOG(ERR, NULL, "Failed to initiate proxy server");
    return -1;
  }

  if (listen(proxy_fd, BACKLOG) == -1) {
    LOG(ERR, NULL, "Failed to listen for connection");
    close(proxy_fd);
    return -1;
  }

  LOG(INFO, NULL, "Listening on port %s", port);
  return proxy_fd;
}

static void event_loop(const int proxy_fd) {
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  memset(&client_addr, 0, addr_len);

  int client_fd = -1;
  while (1) {
    pthread_testcancel();
    client_fd = accept(proxy_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd == -1) {
      LOG(WARN, NULL, "Failed to accept client connection");
      continue;
    }

    char ip[INET_ADDRSTRLEN] = {0};
    if (inet_ntop(client_addr.sin_family, &client_addr.sin_addr, ip,
                  INET_ADDRSTRLEN - 1) == NULL) {
      LOG(WARN, NULL, "Failed to parse client address");
      close(client_fd);
      continue;
    }

    LOG(INFO, NULL, "New connection from %s:%d", ip, client_addr.sin_port);

    // TODO: Implement a client connection queue, in case the # of connection
    // exceeds the MAX_THREADS, accept those new connection and enqueue them and
    // handle them when a client handler exits

    int slot = find_empty_slot();
    if (slot != -1) {
      pthread_mutex_lock(&lock);
      if (pthread_create(&thread_pool[slot], NULL, handler, &client_fd) != 0) {
        LOG(WARN, NULL, "Failed to create handler thread for client");
        close(client_fd);
        continue;
      }

      pthread_detach(thread_pool[slot]);
      thread_count++;
      pthread_mutex_unlock(&lock);
      continue;
    }

    LOG(WARN, NULL,
        "Max number of connection reached! Dropping the connection!");
    close(client_fd);
  }
}

static void cleanup(void *arg) {
  int proxy_fd = *(int *)arg;
  if (proxy_fd != -1)
    close(proxy_fd);
}

void *proxy(void *arg) {
  int proxy_fd = init_proxy((char *)arg);
  if (proxy_fd == -1)
    return NULL;

  pthread_cleanup_push(cleanup, &proxy_fd);

  event_loop(proxy_fd);

  pthread_cleanup_pop(0);
  close(proxy_fd);
  return NULL;
}
