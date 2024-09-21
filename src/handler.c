#include <sys/poll.h>

#include "handler.h"

#define TIMEOUT -1
#define MAX_HTTP_LEN 8192

static int client_handler(const int client_fd, int *server_fd,
                          struct pollfd fds[2]) {
  char buffer[MAX_HTTP_LEN] = {0};
  ssize_t bytes_recv = recv(client_fd, buffer, MAX_HTTP_LEN - 1, 0);
  if (bytes_recv <= 0) {
    if (bytes_recv == -1)
      LOG(ERR, NULL, "Failed to receive from client");

    if (bytes_recv == 0)
      LOG(INFO, NULL, "Client closed the connection!");

    return -1;
  }

  LOG(DBG, NULL, "Received from client %s", buffer);

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
      if (client_handler(client_fd, &server_fd, fds) == -1)
        break;

    if (server_fd != -1 && (fds[1].revents & POLLIN))
      LOG(INFO, NULL, "RECV FROM SERVER");
  }

  if (server_fd != -1)
    close(server_fd);
  close(client_fd);

  return NULL;
}
