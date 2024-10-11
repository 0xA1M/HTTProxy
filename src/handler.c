#include "handler.h"
#include "common.h"

static void cleanup(void *arg) {
  ConnInfo *info = (ConnInfo *)arg;
  if (info == NULL)
    return;

  if (info->fds[0].fd != -1) { // close client fd
    close(info->fds[0].fd);
    info->fds[0].fd = -1;
  }

  if (info->fds[1].fd != -1) { // close server fd
    close(info->fds[1].fd);
    info->fds[1].fd = -1;
  }

  free_req(info->req);
  free_res(info->res);
}

void *handler(void *arg) {
  ConnInfo info = {.fds = {{0}, {0}}, .req = NULL, .res = NULL};
  bool is_TLS = false;

  info.fds[0].fd = *(int *)arg;
  info.fds[0].events = POLLIN;

  info.fds[1].fd = -1;
  info.fds[1].events = POLLIN;

  info.req = (Request *)malloc(sizeof(Request));
  if (info.req == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to request struct");
    close(info.fds[0].fd);
    return NULL;
  }
  memset(info.req, 0, sizeof(Request));

  info.res = (Response *)malloc(sizeof(Response));
  if (info.res == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to request struct");
    free(info.req);
    close(info.fds[0].fd);
    return NULL;
  }
  memset(info.res, 0, sizeof(Response));

  pthread_cleanup_push(cleanup, &info);
  while (1) {
    pthread_testcancel();
    int nfds = info.fds[1].fd != -1 ? 2 : 1;
    int events = poll(info.fds, nfds, TIMEOUT);
    if (events <= 0) {
      if (events == -1)
        LOG(ERR, NULL, "Failed to poll for events");
      if (events == 0)
        LOG(INFO, NULL, "Connection timeout!");
      break;
    }

    if (info.fds[0].revents & POLLIN)
      if (client_handler(info.fds, info.req, &is_TLS) == -1)
        break;

    if (info.fds[1].fd != -1 && (info.fds[1].revents & POLLIN))
      if (server_handler(info.fds, info.res, &is_TLS) == -1)
        break;
  }

  cleanup(&info);
  pthread_cleanup_pop(0);
  remove_thread(pthread_self());
  return NULL;
}
