#include <arpa/inet.h>

#include "common.h"
#include "handler.h"

static int connect_to_server(const char *host) {
  if (host == NULL) {
    LOG(ERR, NULL, "Empty host");
    return -1;
  }

  char *domain = NULL;
  char port[MAX_PORT_LEN] = "80"; // Default port for HTTP

  char *delim = strchr(host, ':');
  if (delim != NULL) {
    size_t domain_len = delim - host;
    domain = strndup(host, domain_len);
    strncpy(port, delim + 1, MAX_PORT_LEN - 1);
    port[MAX_PORT_LEN - 1] = '\0';
  } else {
    domain = strdup(host);
  }

  struct addrinfo hints, *res, *p;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;       // IPv4
  hints.ai_socktype = SOCK_STREAM; // TCP

  int status = getaddrinfo(domain, port, &hints, &res);
  if (status != 0) {
    LOG(ERR, gai_strerror(status), "getaddrinfo failed");
    freeaddrinfo(res);
    free(domain);
    return -1;
  }

  char ip[INET_ADDRSTRLEN] = {0};
  int server_fd = -1;

  for (p = res; p != NULL; p = p->ai_next) {
    // Convert IP to human-readable form
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
    inet_ntop(p->ai_family, &(ipv4->sin_addr), ip, sizeof ip);

    LOG(INFO, NULL, "Attempting to establish a connection to %s(%s):%s", domain,
        ip, port);

    server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (server_fd == -1) {
      LOG(WARN, NULL, "Failed to init server socket");
      continue;
    }

    if (connect(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
      LOG(WARN, NULL, "Failed to connect to server");
      close(server_fd);
      server_fd = -1;
      continue;
    }

    break;
  }

  freeaddrinfo(res);
  free(domain);

  if (p == NULL) {
    LOG(ERR, NULL, "Failed to establish a connection to %s:%s", host, port);
    return -1;
  }

  LOG(INFO, NULL, "Connection established!");
  return server_fd;
}

int client_handler(const int client_fd, int *server_fd, struct pollfd fds[2],
                   Request *req, bool *is_TLS) {
  unsigned char buffer[MAX_HTTP_LEN] = {0};
  long bytes_recv = recv(client_fd, buffer, MAX_HTTP_LEN - 1, 0);
  if (bytes_recv <= 0) {
    if (bytes_recv == -1)
      LOG(ERR, NULL, "Failed to receive from client");

    if (bytes_recv == 0)
      LOG(INFO, NULL, "Client closed the connection!");

    return -1;
  }

  if (*is_TLS && *server_fd != -1) {
    LOG(DBG, NULL, "Received TLS traffic from client (%zu Bytes)", bytes_recv);
    if (forward(*server_fd, buffer, bytes_recv) == -1) {
      LOG(ERR, NULL, "Couldn't forward bytes to server");
      close(*server_fd);
      *server_fd = -1;
      fds[1].fd = *server_fd;
      return -1;
    }
    return 0;
  }

  LOG(DBG, NULL, "Received from client (%zu Bytes): ", bytes_recv);

  if (parse_request(buffer, bytes_recv, req) == -1)
    return -1;

  print_req(req);

  char *host = get_header_value("Host", req->headers, req->headers_count);
  if (host == NULL) {
    LOG(ERR, NULL, "No Host header found, dropping the request!");
    return -1;
  }

  if (*server_fd == -1) {
    *server_fd = connect_to_server(host);
    if (*server_fd == -1)
      return -1;

    fds[1].fd = *server_fd;
  }

  if (strncmp("CONNECT", req->method, 7) == 0) {
    const char *response = "HTTP/1.1 200 Connection Established\r\n"
                           "Proxy-Agent: HTTProxy/1.0\r\n"
                           "\r\n";
    if (forward(client_fd, (unsigned char *)response, strlen(response)) == -1) {
      LOG(ERR, NULL, "Couldn't forward bytes to server");
      close(*server_fd);
      *server_fd = -1;
      fds[1].fd = *server_fd;
      return -1;
    }
    *is_TLS = true;
    return 0;
  }

  if (forward(*server_fd, buffer, bytes_recv) == -1) {
    LOG(ERR, NULL, "Couldn't forward bytes to server");
    close(*server_fd);
    *server_fd = -1;
    fds[1].fd = *server_fd;
    return -1;
  }

  LOG(INFO, NULL, "Bytes successfully forwarded to server");
  return 0;
}
