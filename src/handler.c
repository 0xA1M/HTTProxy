#include "handler.h"
#include "common.h"

int server_handler(const int client_fd, const int server_fd) {
  unsigned char *buffer[MAX_HTTP_LEN] = {0};
  long bytes_recv = recv(server_fd, buffer, MAX_HTTP_LEN - 1, 0);
  if (bytes_recv <= 0)
    return -1;

  long bytes_send = send(client_fd, buffer, bytes_recv, 0);
  if (bytes_send < 0)
    return -1;
  return 0;
}

void *handler(void *arg) {
  int client_fd = *((int *)arg);
  int server_fd = -1;

  struct pollfd fds[2] = {0};

  fds[0].fd = client_fd;
  fds[0].events = POLLIN;

  fds[1].fd = server_fd;
  fds[1].events = POLLIN;

  Request *req = (Request *)malloc(sizeof(Request));
  if (req == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to request struct");
    return NULL;
  }
  memset(req, 0, sizeof(Request));

  bool is_TLS = false;
  while (1) {
    int nfds = server_fd != -1 ? 2 : 1;
    int events = poll(fds, nfds, TIMEOUT);
    if (events <= 0) {
      if (events == -1)
        LOG(ERR, NULL, "Failed to poll for events");

      if (events == 0)
        LOG(INFO, NULL, "Connection timeout!");

      break;
    }

    if (fds[0].revents & POLLIN)
      if (client_handler(client_fd, &server_fd, fds, req, &is_TLS) == -1)
        break;

    if (server_fd != -1 && (fds[1].revents & POLLIN))
      if (server_handler(client_fd, server_fd) == -1)
        break;
  }

  free_req(req);

  if (server_fd != -1)
    close(server_fd);
  close(client_fd);

  remove_thread(pthread_self());
  return NULL;
}
