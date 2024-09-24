#include "common.h"
#include "handler.h"

int client_handler(const int client_fd, int *server_fd, struct pollfd fds[2],
                   Request *req) {
  (void)server_fd;
  (void)fds;
  unsigned char buffer[MAX_HTTP_LEN] = {0};
  long bytes_recv = recv(client_fd, buffer, MAX_HTTP_LEN - 1, 0);
  if (bytes_recv <= 0) {
    if (bytes_recv == -1)
      LOG(ERR, NULL, "Failed to receive from client");

    if (bytes_recv == 0)
      LOG(INFO, NULL, "Client closed the connection!");

    return -1;
  }

  LOG(DBG, NULL, "Received from client (%zu Bytes): ", bytes_recv);

  if (memcmp("CONNECT", buffer, 7) == 0) {
    LOG(DBG, NULL, "Ignored CONNECT request");
    return 0;
  }

  if (parse_request(buffer, bytes_recv, req) == -1)
    return -1;

  print_req(req);

  if (forward(client_fd, buffer, bytes_recv) == -1) {
    LOG(ERR, NULL, "Couldn't forward bytes to server");
    return -1;
  }

  LOG(INFO, NULL, "Bytes successfully forwarded to server");

  free_req(req);
  return 0;
}
