#include <arpa/inet.h>

#include "handler.h"
#include "proxy.h"

#define MAX_PORT_LEN 6 // Max length of the port number
#define BACKLOG 16     // Max members in listening queue

static int init_proxy(const char *port) {
  struct addrinfo hints, *res;

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

  int proxy_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (proxy_fd == -1) {
    LOG(ERR, NULL, "Failed to create proxy server socket");
    freeaddrinfo(res);
    return -1;
  }

  int opt = 1;
  if (setsockopt(proxy_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt) == -1) {
    LOG(ERR, NULL, "Failed to set socket options to allow address reuse");
    freeaddrinfo(res);
    close(proxy_fd);
    return -1;
  }

  if (bind(proxy_fd, res->ai_addr, res->ai_addrlen) == -1) {
    LOG(ERR, NULL, "Failed to bind proxy server socket to address");
    freeaddrinfo(res);
    close(proxy_fd);
    return -1;
  }

  if (listen(proxy_fd, BACKLOG) == -1) {
    LOG(ERR, NULL, "Failed to listen for connection");
    freeaddrinfo(res);
    close(proxy_fd);
    return -1;
  }

  LOG(INFO, NULL, "Listening on port %s", port);

  freeaddrinfo(res);
  return proxy_fd;
}

static void event_loop(const int proxy_fd, bool *run) {
  int client_fd = -1;
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);

  memset(&client_addr, 0, addr_len);

  while (*run) {
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

    pthread_t client_tid = {0};
    if (pthread_create(&client_tid, NULL, handler, &client_fd) != 0) {
      LOG(WARN, NULL, "Failed to create handler thread for client");
      close(client_fd);
      continue;
    }

    pthread_detach(client_tid);
  }
}

void *proxy(void *arg) {
  int proxy_fd = init_proxy((char *)arg);
  if (proxy_fd == -1)
    return NULL;

  bool run = true;
  event_loop(proxy_fd, &run);

  close(proxy_fd);
  return NULL;
}
