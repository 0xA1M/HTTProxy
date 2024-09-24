#include "parser.h"
#include "common.h"

static unsigned char *parse_request_line(const unsigned char *raw,
                                         Request *req) {
  unsigned char *method_end =
      (unsigned char *)memchr(raw, ' ', req->header_size);
  long method_size = method_end - raw;

  req->method = (char *)malloc(method_size + 1);
  if (req->method == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to store method");
    free_req(req);
    return NULL;
  }
  memset(req->method, 0, method_size);
  memcpy(req->method, raw, method_size);
  req->method[method_size] = '\0';

  method_end += 1; // Skip past the SP

  unsigned char *uri_end = (unsigned char *)memchr(
      method_end, ' ', req->header_size - (method_size + 1));
  long uri_size = uri_end - method_end;

  req->uri = (char *)malloc(uri_size + 1);
  if (req->uri == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to store URI");
    free_req(req);
    return NULL;
  }
  memset(req->uri, 0, uri_size);
  memcpy(req->uri, method_end, uri_size);
  req->uri[uri_size] = '\0';

  uri_end += 1; // Skip past the SP

  unsigned char *version_end = (unsigned char *)memchr(
      uri_end, '\r', req->header_size - (method_size + uri_size + 2));
  long version_size = version_end - uri_end;

  req->version = (char *)malloc(version_size + 1);
  if (req->version == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to store HTTP version");
    free_req(req);
    return NULL;
  }
  memset(req->version, 0, version_size);
  memcpy(req->version, uri_end, version_size);
  req->version[version_size] = '\0';

  return version_end + 2; // SKip past the \r\n
}

static int init_header_key(const unsigned char *key, const long key_len,
                           Request *req) {
  req->headers[req->headers_count].key = (char *)malloc(key_len + 1);
  if (req->headers[req->headers_count].key == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to store key field of a header");
    free_req(req);
    return -1;
  }
  memset(req->headers[req->headers_count].key, 0, key_len);
  memcpy(req->headers[req->headers_count].key, key, key_len);
  req->headers[req->headers_count].key[key_len] = '\0';
  return 0;
}

static int init_header_value(const unsigned char *value, const long value_len,
                             Request *req) {
  req->headers[req->headers_count].value = (char *)malloc(value_len + 1);
  if (req->headers[req->headers_count].value == NULL) {
    LOG(ERR, NULL,
        "Failed to allocate memory to store value field of a header");
    free_req(req);
    return -1;
  }
  memset(req->headers[req->headers_count].value, 0, value_len);
  memcpy(req->headers[req->headers_count].value, value,
         value_len); // Skip past the : and SP
  req->headers[req->headers_count].value[value_len] = '\0';
  return 0;
}

static unsigned char *parse_headers(unsigned char *line,
                                    unsigned char *headers_end, Request *req) {
  while (req->headers_count < MAX_HEADERS) {
    // Find where this particular header ends
    unsigned char *next_line =
        (unsigned char *)memmem(line, headers_end - line, "\r\n", 2);
    if (next_line == NULL)
      break; // No more headers to process

    // Calculate it's length
    long header_len = next_line - line;
    if (header_len == 0)
      break; // Reached a CRLF, marking the end of the headers

    unsigned char *delim = memchr(line, ':', header_len);
    if (delim != NULL) {
      long key_len = delim - line;
      long value_len = header_len - (key_len + 2); // Account for the : and SP

      if (init_header_key(line, key_len, req) == -1)
        return NULL;

      if (init_header_value(delim + 2, value_len, req) == -1)
        return NULL;

      req->headers_count++;
    }

    line = next_line + 2; // Skip past the CRLF
  }

  return line + 2; // Skip the last CRLF
}

static char *get_header_value(char *target, const Header *headers,
                              const size_t headers_count) {
  for (size_t i = 0; i < headers_count; i++)
    if (memcmp(target, headers[i].key, strlen(target)) == 0)
      return headers[i].value;

  return NULL;
}

static int parse_body(const unsigned char *body, Request *req) {
  // According to RFC 9110 a request message has an optional message body if
  // one of the following conditions is met:
  // 1. If Content-Length is present, read the exact number of bytes.
  // 2. If Transfer-Encoding: chunked is present, parse the chunks.
  // If neither is present, assume there is no body.
  char *content_length_str =
      get_header_value("Content-Length", req->headers, req->headers_count);
  if (content_length_str != NULL) {
    int content_length = strtol(content_length_str, NULL, 10);
    req->body = (unsigned char *)malloc(content_length + 1);
    if (req->body == NULL) {
      LOG(ERR, NULL, "Failed to allocate memory to store request body");
      free(req);
      return -1;
    }
    memset(req->body, 0, content_length);
    memcpy(req->body, body, content_length);
    req->body_size = content_length;
    return 0;
  }

  char *transfer_encoding =
      get_header_value("Transfer-Encoding", req->headers, req->headers_count);
  if (transfer_encoding != NULL) {
    // TODO
  }

  req->body = NULL;
  return 0;
}

int parse_request(const unsigned char *raw, const long len, Request *req) {
  memset(req, 0, sizeof(Request));

  // Search for the last '\r\n\r\n' indicating the end of the header section
  unsigned char *headers_end = (unsigned char *)memmem(raw, len, "\r\n\r\n", 4);
  if (headers_end == NULL) {
    // TODO: Return a Invalid Request page
    LOG(ERR, NULL, "Invalid Request");
    free_req(req);
    return -1;
  }
  // Move the headers_end past the last '\r\n\r\n'.
  headers_end += 4;

  // Calculate how many bytes are between the raw pointer and the headers_end
  // pointer thus determining the header section size of the request
  req->header_size = headers_end - raw;

  unsigned char *headers_start = parse_request_line(raw, req);
  if (headers_start == NULL)
    return -1;

  unsigned char *body_start = parse_headers(headers_start, headers_end, req);
  if (body_start == NULL)
    return -1;

  if (parse_body(body_start, req) == -1)
    return -1;

  return 0;
}
