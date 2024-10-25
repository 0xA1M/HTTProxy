#include <arpa/inet.h>

#include "common.h"
#include "handler.h"

typedef struct Server_Info {
  struct addrinfo *res;
  int *server_fd;
} Server_Info;

static void clean(void *arg) {
  Server_Info *info = (Server_Info *)arg;
  if (*info->server_fd != -1) {
    close(*info->server_fd);
    *info->server_fd = -1;
  }

  freeaddrinfo(info->res);
  info->res = NULL;
}

static int establish_connection(const char *host) {
  char hostname[MAX_HOSTNAME_LEN] = {0};
  char port[MAX_PORT_LEN] = "80";

  char *delim = strchr(host, ':');
  if (delim) {
    size_t len = delim - host;
    strncpy(hostname, host, len);
    hostname[MAX_HOSTNAME_LEN - 1] = '\0';
    strncpy(port, delim + 1, MAX_PORT_LEN - 1);
    port[MAX_PORT_LEN - 1] = '\0';
  } else {
    strncpy(hostname, host, MAX_HOSTNAME_LEN - 1);
    hostname[MAX_HOSTNAME_LEN - 1] = '\0';
  }

  struct addrinfo hints, *res, *p;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

  int status = getaddrinfo(hostname, port, &hints, &res);
  if (status != 0) {
    LOG(ERR, gai_strerror(status), "getaddrinfo failed");
    return -1;
  }

  char ip[INET6_ADDRSTRLEN] = {0};
  int server_fd = -1;
  Server_Info info = {res, &server_fd};
  pthread_cleanup_push(clean, &info);
  for (p = res; p != NULL; p = p->ai_next) {
    pthread_testcancel();
    void *addr = NULL;

    if (p->ai_family == AF_INET) { // IPv4
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      addr = &(ipv4->sin_addr);
    } else { // IPv6
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
      addr = &(ipv6->sin6_addr);
    }

    inet_ntop(p->ai_family, addr, ip, sizeof ip);
    LOG(INFO, NULL, "Attempting to establish a connection to %s(%s:%s)",
        hostname, ip, port);

    server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (server_fd == -1)
      continue;

    if (connect(server_fd, p->ai_addr, p->ai_addrlen) != -1)
      break; // Connection successfully established

    close(server_fd);
    server_fd = -1;
  }

  freeaddrinfo(res);
  res = NULL;
  pthread_cleanup_pop(0);
  if (p == NULL) {
    LOG(ERR, NULL, "Failed to establish a connection to %s:%s", host, port);
    return -1;
  }

  LOG(INFO, NULL, "Connection to %s:%s has been established!", hostname, port);
  return server_fd;
}

int client_handler(struct pollfd *fds, Request *req, bool *is_TLS) {
  unsigned char buffer[MAX_HTTP_LEN] = {0};
  long bytes_recv = recv(fds[0].fd, buffer, MAX_HTTP_LEN - 1, 0);
  if (bytes_recv <= 0) {
    if (bytes_recv == -1)
      LOG(ERR, NULL, "Failed to receive from client");
    if (bytes_recv == 0)
      LOG(INFO, NULL, "Client closed the connection!");
    return -1;
  }

  if (*is_TLS && fds[1].fd != -1) {
    LOG(DBG, NULL, "Received TLS traffic from client (%zu Bytes)", bytes_recv);
    if (forward(fds[1].fd, buffer, bytes_recv) == -1) {
      LOG(ERR, NULL, "Couldn't forward bytes to server");
      return -1;
    }
    return 0;
  }

  LOG(DBG, NULL, "Received from client (%ld Bytes): ", bytes_recv);

  if (parse_request(buffer, bytes_recv, req) == -1)
    return -1;

  print_req(req);

  char *host = get_header_value("Host", req->headers, req->headers_count);
  if (host == NULL) {
    LOG(ERR, NULL, "No Host header found, dropping the request!");
    return -1;
  }

  if (fds[1].fd == -1) {
    fds[1].fd = establish_connection(host);
    if (fds[1].fd == -1)
      return -1;
  }

  if (strncmp("CONNECT", req->method, 7) == 0) {
    const char *response = "HTTP/1.1 200 Connection Established\r\n"
                           "Proxy-Agent: HTTProxy/1.0\r\n"
                           "\r\n";
    if (forward(fds[0].fd, (unsigned char *)response, strlen(response)) == -1) {
      LOG(ERR, NULL, "Couldn't forward bytes to server");
      return -1;
    }
    *is_TLS = true;
    return 0;
  }

  if (forward(fds[1].fd, buffer, bytes_recv) == -1) {
    LOG(ERR, NULL, "Couldn't forward bytes to server");
    return -1;
  }

  LOG(INFO, NULL, "Bytes successfully forwarded to server");
  return 0;
}
