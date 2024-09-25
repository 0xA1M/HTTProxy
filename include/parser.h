#ifndef PARSER_H
#define PARSER_H

/* Standard Library */
#include <stdio.h>

/* Constant */
#define MAX_HEADERS 128

/* Data Structures */
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

  size_t header_size; // The whole request header size

  // Body (Optional)
  unsigned char *body;
  size_t body_size;
} Request;

int parse_request(const unsigned char *raw, const long len, Request *req);

#endif /* PARSER_H */
