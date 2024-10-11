#include "common.h"
#include "handler.h"

int server_handler(const struct pollfd *fds, Response *res, bool *is_TLS) {
  unsigned char buffer[MAX_HTTP_LEN] = {0};
  long bytes_recv = recv(fds[1].fd, buffer, MAX_HTTP_LEN - 1, 0);
  if (bytes_recv <= 0) {
    if (bytes_recv == -1)
      LOG(ERR, NULL, "Failed to receive from server");
    if (bytes_recv == 0)
      LOG(INFO, NULL, "Server closed the connection!");
    return -1;
  }

  if (*is_TLS) {
    LOG(DBG, NULL, "Received TLS traffic from server (%zu Bytes)", bytes_recv);
    if (forward(fds[0].fd, buffer, bytes_recv) == -1) {
      LOG(ERR, NULL, "Couldn't forward bytes to client");
      return -1;
    }
    return 0;
  }

  LOG(DBG, NULL, "Received from server (%zu Bytes): \n%s\n ", bytes_recv,
      buffer);

  (void)res;
  /* if (parse_response(buffer, bytes_recv, res) == -1)
    return -1;

  print_res(res); */

  if (forward(fds[0].fd, buffer, bytes_recv) == -1) {
    LOG(ERR, NULL, "Couldn't forward bytes to client");
    return -1;
  }

  LOG(INFO, NULL, "Bytes successfully forwarded to client");
  return 0;
}
