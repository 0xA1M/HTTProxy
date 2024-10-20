#include "common.h"

#define CRLF 2
#define CHUNK_SIZE_LEN(size) ((size == 0) ? 1 : 16)

static char *normalize_uri(const char *uri, const long uri_len,
                           const char *method) {
  bool is_options = strncmp("OPTIONS", method, 7) == 0;
  if (uri == NULL || is_options) {
    if (is_options)
      return strndup("*", 1);

    return strndup("/", 1);
  }

  const char *path = (char *)memmem(uri, uri_len, "://", 3);
  if (path != NULL) {
    path += 3; // Skip past the schema (http:// or https://)
    path = (char *)memchr(path, '/', uri_len - (path - uri)); // Skip the domain
    if (path == NULL)
      return strndup("/", 1); // No path found, assume root
  } else {
    path = uri; // If there's no scheme, assume it's already a relative URI
  }

  // If the URI is malformed or empty, default to "/"
  if (path == NULL || *path == '\0')
    return strndup("/", 1);

  // Look for fragments ('#') and ignore everything after it
  char *frag = (char *)memchr(path, '#', uri_len - (path - uri));
  long path_len = strlen(path);
  if (frag != NULL)
    path_len = frag - path;

  char *normalized_path = (char *)malloc(path_len + 1);
  if (normalized_path == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory for normalized path");
    return NULL;
  }

  // Copy the path and null-terminate it
  memcpy(normalized_path, path, path_len);
  normalized_path[path_len] = '\0';

  return normalized_path;
}

static unsigned char *parse_request_line(const unsigned char *raw,
                                         Request *req) {
  if (raw == NULL || req == NULL)
    return NULL;

  unsigned char *method_end =
      (unsigned char *)memchr(raw, ' ', req->header_size);
  long method_size = method_end - raw;

  req->method = (char *)malloc(method_size + 1);
  if (req->method == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to store method");
    return NULL;
  }
  memcpy(req->method, raw, method_size);
  req->method[method_size] = '\0';

  method_end += 1; // Skip past the SP

  unsigned char *uri_end = (unsigned char *)memchr(
      method_end, ' ', req->header_size - (method_size + 1));
  long uri_size = uri_end - method_end;

  char *raw_uri = (char *)malloc(uri_size + 1);
  if (raw_uri == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to store URI");
    return NULL;
  }
  memcpy(raw_uri, method_end, uri_size);
  raw_uri[uri_size] = '\0';

  // Normalize the URI
  req->uri = normalize_uri(raw_uri, uri_size, req->method);
  if (req->uri == NULL) {
    free(raw_uri);
    raw_uri = NULL;
    return NULL;
  }
  free(raw_uri);
  raw_uri = NULL;

  uri_end += 1; // Skip past the SP

  unsigned char *version_end = (unsigned char *)memchr(
      uri_end, '\r', req->header_size - (method_size + uri_size + 2));
  long version_size = version_end - uri_end;

  req->version = (char *)malloc(version_size + 1);
  if (req->version == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to store HTTP version");
    return NULL;
  }
  memcpy(req->version, uri_end, version_size);
  req->version[version_size] = '\0';

  return version_end + 2; // Skip past the \r\n, the beginning of the headers
}

static unsigned char *parse_response_line(const unsigned char *raw,
                                          Response *res) {
  if (raw == NULL || res == NULL)
    return NULL;

  unsigned char *version_end =
      (unsigned char *)memchr(raw, ' ', res->header_size);
  long version_size = version_end - raw;

  res->version = (char *)malloc(version_size + 1);
  if (res->version == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to store HTTP version");
    return NULL;
  }
  memcpy(res->version, raw, version_size);
  res->version[version_size] = '\0';

  version_end += 1; // Skip past the SP

  unsigned char *status_code = (unsigned char *)memchr(
      version_end, ' ', res->header_size - (version_size + 1));
  long status_code_size = status_code - version_end;

  res->status_code = (char *)malloc(status_code_size + 1);
  if (res->version == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to store status code");
    return NULL;
  }
  memcpy(res->status_code, version_end, status_code_size);
  res->status_code[status_code_size] = '\0';

  status_code += 1; // Skip past the SP

  unsigned char *reason_phrase = (unsigned char *)memchr(
      status_code, '\r',
      res->header_size - (version_size + status_code_size + 2));
  long reason_phrase_size = reason_phrase - status_code;

  res->reason_phrase = (char *)malloc(reason_phrase_size + 1);
  if (res->reason_phrase == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to store reason phrase");
    return NULL;
  }
  memcpy(res->reason_phrase, status_code, reason_phrase_size);
  res->reason_phrase[reason_phrase_size] = '\0';

  return reason_phrase + 2; // Skip past the \r\n, the beginning of the headers
}

static int init_header_key(const unsigned char *key, const long key_len,
                           Header *headers, const size_t headers_count) {
  headers[headers_count].key = (char *)malloc(key_len + 1);
  if (headers[headers_count].key == NULL) {
    LOG(ERR, NULL, "Failed to allocate memory to store key field of a header");
    return -1;
  }
  memcpy(headers[headers_count].key, key, key_len);
  headers[headers_count].key[key_len] = '\0';
  return 0;
}

static int init_header_value(const unsigned char *value, const long value_len,
                             Header *headers, const size_t headers_count) {
  headers[headers_count].value = (char *)malloc(value_len + 1);
  if (headers[headers_count].value == NULL) {
    LOG(ERR, NULL,
        "Failed to allocate memory to store value field of a header");
    return -1;
  }
  memcpy(headers[headers_count].value, value,
         value_len); // Skip past the : and SP
  headers[headers_count].value[value_len] = '\0';
  return 0;
}

static unsigned char *parse_headers(unsigned char *line,
                                    unsigned char *headers_end, Header *headers,
                                    size_t *headers_count) {
  while (*headers_count < MAX_HEADERS) {
    // Find where this particular header ends
    unsigned char *next_line =
        (unsigned char *)memmem(line, headers_end - line, "\r\n", 2);
    if (next_line == NULL)
      break; // No more headers to process

    // Calculate it's length
    long header_len = next_line - line;
    if (header_len == 0)
      break; // Reached the last CRLF, marking the end of the headers

    unsigned char *delim = memchr(line, ':', header_len);
    if (delim != NULL) {
      long key_len = delim - line;
      long value_len = header_len - (key_len + 2); // Account for the : and SP

      if (init_header_key(line, key_len, headers, *headers_count) == -1)
        return NULL;

      if (init_header_value(delim + 2, value_len, headers, *headers_count) ==
          -1)
        return NULL;

      (*headers_count)++;
    }

    line = next_line + 2; // Skip past the CRLF
  }

  return line + 2; // Skip the last CRLF
}

static int parse_req_body(const unsigned char *body, const size_t body_len,
                          Request *req) {
  /*
   * Rules for determining if an HTTP request has a body (RFC 9110):
   *
   * 1. Method-specific Rules:
   *    - Some methods (e.g., POST and PUT) often have a body.
   *    - Other methods (e.g., GET, HEAD, DELETE, CONNECT) typically don't have
   * a body.
   *    - However, any method MAY include a body if properly indicated.
   *
   * 2. Content-Length Header:
   *    - If present, its value indicates the exact length of the body in
   * octets.
   *    - A Content-Length header with any value (including 0) indicates a body
   * is present.
   *
   * 3. Transfer-Encoding Header:
   *    - If present, it indicates a body with chunked encoding.
   *    - The presence of this header always indicates a body, even if it's
   * empty.
   *
   * 4. Expect: 100-continue Header:
   *    - If present, it usually indicates that the request has a body.
   *    - The client will wait for a 100 Continue response before sending the
   * body.
   *
   * 5. Multipart Content-Type:
   *    - If the Content-Type header indicates a multipart type, a body is
   * present.
   *
   * 6. Order of Precedence:
   *    1. Transfer-Encoding header
   *    2. Content-Length header
   *    3. Method semantics and other headers (e.g., Content-Type, Expect)
   *
   * 7. Special Cases:
   *    - TRACE method MUST NOT include a body.
   *    - If both Content-Length and Transfer-Encoding are present,
   * Transfer-Encoding takes precedence.
   *
   * Note: In a proxy server implementation, you should be prepared to handle
   * all these cases, including chunked transfer encoding. Always check headers
   * to determine if a body is present, regardless of the HTTP method used.
   */
  char *content_length_str =
      get_header_value("Content-Length", req->headers, req->headers_count);
  if (content_length_str != NULL) {
    int content_length = strtol(content_length_str, NULL, 10);
    if (content_length == 0) {
      req->body = NULL;
      req->body_size = 0;
      return 0;
    }

    req->body = (unsigned char *)malloc(content_length);
    if (req->body == NULL) {
      LOG(ERR, NULL, "Failed to allocate memory to store request body");
      return -1;
    }
    memcpy(req->body, body, content_length);
    req->body_size = content_length;
    return 0;
  }

  char *transfer_encoding =
      get_header_value("Transfer-Encoding", req->headers, req->headers_count);
  if (transfer_encoding != NULL &&
      memcmp("chunked", transfer_encoding, 7) == 0) {
    int chunk_size = get_chunk_size(body, body_len);
    if (chunk_size == -1)
      return -1;

    if (chunk_size == 0) {
      req->body = (unsigned char *)malloc(
          chunk_size + CHUNK_SIZE_LEN(chunk_size) + CRLF * 2);
      if (req->body == NULL) {
        LOG(ERR, NULL, "Failed to allocate memory to store request body");
        return -1;
      }
      memcpy(req->body, body,
             chunk_size + CHUNK_SIZE_LEN(chunk_size) + CRLF * 2);
      req->body_size = chunk_size + CHUNK_SIZE_LEN(chunk_size) + CRLF * 2;
      req->is_chunked = false;
      return 0;
    }

    req->body = (unsigned char *)malloc(chunk_size +
                                        CHUNK_SIZE_LEN(chunk_size) + CRLF * 2);
    if (req->body == NULL) {
      LOG(ERR, NULL, "Failed to allocate memory to store request body");
      return -1;
    }
    memcpy(req->body, body, chunk_size + CHUNK_SIZE_LEN(chunk_size) + CRLF * 2);
    req->body_size = chunk_size + CHUNK_SIZE_LEN(chunk_size) + CRLF * 2;
    req->is_chunked = true;
    return 0;
  }

  req->body = NULL;
  req->body_size = 0;
  return 0;
}

static int parse_res_body(const unsigned char *body, const size_t body_len,
                          Response *res) {
  /*
   * Rules for determining if an HTTP response has a body (RFC 9110):
   *
   * 1. Status Code Rules:
   *    - Responses with status codes 1xx (Informational), 204 (No Content),
   *      and 304 (Not Modified) MUST NOT include a body.
   *    - All other status codes indicate the presence of a body, even if it's
   * empty.
   *
   * 2. Content-Length Header:
   *    - If present, its value indicates the exact length of the body in
   * octets.
   *    - A Content-Length header with any value (including 0) indicates a body
   * is present.
   *
   * 3. Transfer-Encoding Header:
   *    - If present, it indicates a body with chunked encoding.
   *    - The presence of this header always indicates a body, even if it's
   * empty.
   *
   * 4. Server Closing the Connection:
   *    - In some cases, the server may indicate the end of the body by closing
   * the connection.
   *    - This applies when none of the above conditions are met.
   *
   * 5. Special Case for HEAD Requests:
   *    - Responses to HEAD requests never have a body, even if headers indicate
   * otherwise.
   *    - The headers should be the same as if the request was a GET.
   *
   * 6. Order of Precedence:
   *    1. Status Code (for 1xx, 204, 304)
   *    2. Transfer-Encoding header
   *    3. Content-Length header
   *    4. Server closing the connection
   *
   * Note: In a proxy server implementation, you should be prepared to handle
   * all these cases, including chunked transfer encoding and connection
   * closure.
   */
  char *content_length_str =
      get_header_value("Content-Length", res->headers, res->headers_count);
  if (content_length_str != NULL) {
    int content_length = strtol(content_length_str, NULL, 10);
    if (content_length == 0) {
      res->body = NULL;
      res->body_size = 0;
      return 0;
    }

    res->body = (unsigned char *)malloc(content_length);
    if (res->body == NULL) {
      LOG(ERR, NULL, "Failed to allocate memory to store response body");
      return -1;
    }
    memcpy(res->body, body, content_length);
    res->body_size = content_length;
    return 0;
  }

  char *transfer_encoding =
      get_header_value("Transfer-Encoding", res->headers, res->headers_count);
  if (transfer_encoding != NULL &&
      memcmp("chunked", transfer_encoding, 7) == 0) {
    int chunk_size = get_chunk_size(body, body_len);
    if (chunk_size == -1)
      return -1;

    if (chunk_size == 0) {
      res->body = (unsigned char *)malloc(
          chunk_size + CHUNK_SIZE_LEN(chunk_size) + CRLF * 2);
      if (res->body == NULL) {
        LOG(ERR, NULL, "Failed to allocate memory to store response body");
        return -1;
      }
      memcpy(res->body, body,
             chunk_size + CHUNK_SIZE_LEN(chunk_size) + CRLF * 2);
      res->body_size = chunk_size + CHUNK_SIZE_LEN(chunk_size) + CRLF * 2;
      res->is_chunked = false;
      return 0;
    }

    res->body = (unsigned char *)malloc(chunk_size +
                                        CHUNK_SIZE_LEN(chunk_size) + CRLF * 2);
    if (res->body == NULL) {
      LOG(ERR, NULL, "Failed to allocate memory to store response body");
      return -1;
    }
    memcpy(res->body, body, chunk_size + CHUNK_SIZE_LEN(chunk_size) + CRLF * 2);
    res->body_size = chunk_size + CHUNK_SIZE_LEN(chunk_size) + CRLF * 2;
    res->is_chunked = true;
    return 0;
  }

  res->body = NULL;
  res->body_size = 0;
  return 0;
}

int parse_request(const unsigned char *raw, const long len, Request *req) {
  if (req->is_chunked) {
    free(req->body);
    req->body = NULL;

    return parse_req_body(raw, len, req);
  }

  memset(req, 0, sizeof(Request));

  // Search for the last '\r\n\r\n' indicating the end of the header section
  unsigned char *headers_end = (unsigned char *)memmem(raw, len, "\r\n\r\n", 4);
  if (headers_end == NULL) {
    LOG(ERR, NULL, "Invalid Request");
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

  unsigned char *body_start = parse_headers(headers_start, headers_end,
                                            req->headers, &req->headers_count);
  if (body_start == NULL)
    return -1;

  if (parse_req_body(body_start, len - req->header_size, req) == -1)
    return -1;

  return 0;
}

int parse_response(const unsigned char *raw, const long len, Response *res) {
  if (res->is_chunked) {
    free(res->body);
    res->body = NULL;

    return parse_res_body(raw, len, res) == -1 ? -1 : 0;
  }

  memset(res, 0, sizeof(Response));

  // Search for the last '\r\n\r\n' indicating the end of the header section
  unsigned char *headers_end = (unsigned char *)memmem(raw, len, "\r\n\r\n", 4);
  if (headers_end == NULL) {
    LOG(ERR, NULL, "Invalid Request");
    return -1;
  }
  // Move the headers_end past the last '\r\n\r\n'.
  headers_end += 4;

  // Calculate how many bytes are between the raw pointer and the headers_end
  // pointer thus determining the header section size of the response
  res->header_size = headers_end - raw;

  unsigned char *headers_start = parse_response_line(raw, res);
  if (headers_start == NULL)
    return -1;

  unsigned char *body_start = parse_headers(headers_start, headers_end,
                                            res->headers, &res->headers_count);
  if (body_start == NULL)
    return -1;

  if (parse_res_body(body_start, len - res->header_size, res) == -1)
    return -1;

  return 0;
}
