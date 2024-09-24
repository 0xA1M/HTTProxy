#include "handler.h"
#include "common.h"

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
      if (client_handler(client_fd, &server_fd, fds, req) == -1)
        break;

    if (server_fd != -1 && (fds[1].revents & POLLIN))
      LOG(INFO, NULL, "No server yet -_-");
  }

  if (server_fd != -1)
    close(server_fd);
  close(client_fd);

  remove_thread(pthread_self());

  return NULL;
}
