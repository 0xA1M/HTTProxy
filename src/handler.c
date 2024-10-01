#include "handler.h"
#include "common.h"

typedef struct ClientInfo {
  int client_fd;
  int server_fd;
} ClientInfo;

static void cleanup(void *arg) {
  ClientInfo *info = (ClientInfo *)arg;
  if (info->client_fd != -1)
    close(info->client_fd);
  if (info->server_fd != -1)
    close(info->server_fd);
}

void *handler(void *arg) {
  ClientInfo info = {*(int *)arg, -1};
  // bool is_TLS = false;

  struct pollfd fds[2] = {0};

  fds[0].fd = info.client_fd;
  fds[0].events = POLLIN;

  fds[1].fd = -1;
  fds[1].events = POLLIN;

  pthread_cleanup_push(cleanup, &info);
  while (1) {
    int nfds = info.server_fd != -1 ? 2 : 1;
    int events = poll(fds, nfds, TIMEOUT);
    if (events <= 0) {
      if (events == -1)
        LOG(ERR, NULL, "Failed to poll for events");

      if (events == 0)
        LOG(INFO, NULL, "Connection timeout!");

      break;
    }

    if (fds[0].revents & POLLIN)
      LOG(DBG, NULL, "Received from client!");
    break;

    if (info.server_fd != -1 && (fds[1].revents & POLLIN))
      LOG(DBG, NULL, "Received from server!");

    pthread_testcancel();
  }

  pthread_cleanup_pop(0);
  remove_thread(pthread_self());
  return NULL;
}
