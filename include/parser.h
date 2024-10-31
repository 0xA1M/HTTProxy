#ifndef PARSER_H
#define PARSER_H

/* Standard Library */
#include <stdio.h>

/* Constant */
#define MAX_HEADERS 256

/* Data Structure */
typedef struct Header {
  char *key;
  char *value;
} Header;

typedef struct Request {
  // Request Line
  char *method;
  char *uri;
  char *version;

  // Headers
  Header headers[MAX_HEADERS];
  size_t headers_count;

  // Size of the headers and the request line
  size_t header_size;

  // Partial recv flag
  bool is_partial;
  size_t partial_recv_size;

  // Chunked transfer encoding flag
  bool is_chunked;

  // Content Type
  char *content_type;
  bool is_text;

  // Content Encoding
  char *content_encoding;

  // Body (optional)
  unsigned char *body;
  size_t body_size;
} Request;

typedef struct Response {
  char *version;
  char *status_code;
  char *reason_phrase;

  Header headers[MAX_HEADERS];
  size_t headers_count;

  // Size of the headers and the response line
  size_t header_size;

  // Partial recv flag
  bool is_partial;
  size_t partial_recv_size;

  // Chunked transfer encoding flag
  bool is_chunked;

  // Content Type
  char *content_type;
  bool is_text;

  // Content Encoding
  char *content_encoding;

  // Body
  unsigned char *body;
  size_t body_size;
} Response;

int parse_request(const unsigned char *raw, const size_t len, Request *req);

int parse_response(const unsigned char *raw, const size_t len, Response *res);

#endif /* PARSER_H */
