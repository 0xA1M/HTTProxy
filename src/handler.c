#include "handler.h"
#include "common.h"

static int server_handler(const int client_fd, const int server_fd) {
  unsigned char *buffer[MAX_HTTP_LEN] = {0};
  long bytes_recv = recv(server_fd, buffer, MAX_HTTP_LEN - 1, 0);
  if (bytes_recv <= 0)
    return -1;

  long bytes_send = send(client_fd, buffer, bytes_recv, 0);
  if (bytes_send < 0)
    return -1;
  return 0;
}

static void cleanup(void *arg) {
  ClientInfo *info = (ClientInfo *)arg;
  if (info->client_fd != -1)
    close(info->client_fd);
  if (info->server_fd != -1)
    close(info->server_fd);
  free_req(info->req);
}

void *handler(void *arg) {
  ClientInfo info = {*(int *)arg, -1, NULL};
  bool is_TLS = false;

  struct pollfd fds[2] = {0};

  fds[0].fd = info.client_fd;
  fds[0].events = POLLIN;

  fds[1].fd = -1;
  fds[1].events = POLLIN;

  info.req = (Request *)malloc(sizeof(Request));
  if (info.req == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to request struct");
    close(info.client_fd);
    return NULL;
  }
  memset(info.req, 0, sizeof(Request));

  Response *res = (Response *)malloc(sizeof(Response));
  if (res == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to request struct");
    free(info.req);
    close(info.client_fd);
    return NULL;
  }
  memset(res, 0, sizeof(Response));

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
      if (client_handler(info.client_fd, &info.server_fd, fds, info.req,
                         &is_TLS) == -1)
        break;

    if (info.server_fd != -1 && (fds[1].revents & POLLIN))
      if (server_handler(info.client_fd, info.server_fd) == -1)
        break;

    pthread_testcancel();
  }

  cleanup(&info);
  pthread_cleanup_pop(0);
  remove_thread(pthread_self());
  return NULL;
}
