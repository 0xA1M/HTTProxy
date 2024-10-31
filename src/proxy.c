#include <arpa/inet.h>

#include "common.h"
#include "handler.h"
#include "proxy.h"

#define BACKLOG 32 // Max members in listening queue

static void cleanup(void *arg) {
  int *proxy_fd = (int *)arg;
  if (*proxy_fd != -1)
    close(*proxy_fd);
}

static int init_proxy(const char *port) {
  struct addrinfo hints, *res, *p;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // TCP
  hints.ai_flags = AI_PASSIVE;     // Use local ip

  int status = getaddrinfo(NULL, port, &hints, &res);
  if (status != 0) {
    LOG(ERR, gai_strerror(status), "getaddrinfo failed");
    return -1;
  }

  int proxy_fd = -1, opt = 1;
  for (p = res; p != NULL; p = p->ai_next) {
    proxy_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (proxy_fd == -1)
      continue;

    if (setsockopt(proxy_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt) ==
        -1) {
      LOG(ERR, NULL, "Failed to set socket options to allow address reuse");
      freeaddrinfo(res);
      res = NULL;
      close(proxy_fd);
      return -1;
    }

    if (bind(proxy_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(proxy_fd);
      proxy_fd = -1;
      continue;
    }

    break;
  }

  freeaddrinfo(res);
  res = NULL;
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
  struct sockaddr_storage client_addr;
  socklen_t addr_len = sizeof(client_addr);
  int client_fd = -1;

  while (1) {
    pthread_testcancel();

    memset(&client_addr, 0, addr_len);
    client_fd = accept(proxy_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd == -1) {
      LOG(WARN, NULL, "Failed to accept client connection");
      continue;
    }

    char ip[INET6_ADDRSTRLEN] = {0};
    if (client_addr.ss_family == AF_INET) {
      struct sockaddr_in *addr = (struct sockaddr_in *)&client_addr;
      if (inet_ntop(AF_INET, &addr->sin_addr, ip, INET_ADDRSTRLEN) == NULL) {
        LOG(WARN, NULL, "Failed to parse client address");
        close(client_fd);
        client_fd = -1;
        continue;
      }

      LOG(INFO, NULL, "New connection from %s:%d", ip, ntohs(addr->sin_port));
    } else if (client_addr.ss_family == AF_INET6) {
      struct sockaddr_in6 *addr = (struct sockaddr_in6 *)&client_addr;
      if (inet_ntop(AF_INET6, &addr->sin6_addr, ip, INET6_ADDRSTRLEN) == NULL) {
        LOG(WARN, NULL, "Failed to parse client address");
        close(client_fd);
        client_fd = -1;
        continue;
      }

      LOG(INFO, NULL, "New connection from %s:%d", ip, ntohs(addr->sin6_port));
    } else {
      LOG(WARN, NULL, "Unknown address family");
      close(client_fd);
      client_fd = -1;
      continue;
    }

    // TODO: Implement a client connection queue, in case the # of connection
    // exceeds the MAX_THREADS, accept those new connection and enqueue them and
    // handle them when a client handler exits

    int slot = find_empty_slot();
    if (slot != -1) {
      if (pthread_create(&thread_pool[slot], NULL, handler, &client_fd) != 0) {
        LOG(WARN, NULL, "Failed to create handler thread for client");
        close(client_fd);
        client_fd = -1;
        continue;
      }
      pthread_detach(thread_pool[slot]);

      pthread_mutex_lock(&lock);
      thread_count++;
      pthread_mutex_unlock(&lock);
      continue;
    }

    LOG(WARN, NULL,
        "Max number of connection reached! Dropping the connection!");
    close(client_fd);
    client_fd = -1;
  }
}

void *proxy(void *arg) {
  char *endptr = NULL;
  const long port = strtol((char *)arg, &endptr, 10);
  if (endptr == (char *)arg || *endptr != '\0' || port < 0 || port > 65535) {
    LOG(ERR, NULL, "Invalid port number");
    return NULL;
  }

  int proxy_fd = init_proxy((char *)arg);
  if (proxy_fd == -1) // Failed to create proxy server
    return NULL;
  pthread_cleanup_push(cleanup, &proxy_fd);

  event_loop(proxy_fd);
  close(proxy_fd);

  pthread_cleanup_pop(0);
  return NULL;
}
