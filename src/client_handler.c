#include "common.h"
#include "handler.h"

int client_handler(const int client_fd, int *server_fd, struct pollfd fds[2]) {
  unsigned char buffer[MAX_HTTP_LEN] = {0};
  ssize_t bytes_recv = recv(client_fd, buffer, MAX_HTTP_LEN - 1, 0);
  if (bytes_recv <= 0) {
    if (bytes_recv == -1)
      LOG(ERR, NULL, "Failed to receive from client");

    if (bytes_recv == 0)
      LOG(INFO, NULL, "Client closed the connection!");

    return -1;
  }

  LOG(DBG, NULL, "Received from client (%zu Bytes): \n%s", bytes_recv, buffer);

  if (forward(client_fd, buffer, bytes_recv) == -1) {
    LOG(ERR, NULL, "Couldn't forward bytes to server");
    return -1;
  }

  LOG(INFO, NULL, "Bytes successfully forwarded to server");
  return 0;
}
