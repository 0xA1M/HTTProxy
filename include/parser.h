#ifndef PARSER_H
#define PARSER_H

/* Standard Library */
#include <stdio.h>

/* Constant */
#define MAX_HEADERS 128

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
  unsigned int headers_count;

  // Size of the headers and the request line
  unsigned int header_size;

  // Body (optional)
  unsigned char *body;
  unsigned int body_size;
} Request;

typedef struct Response {
  char *version;
  unsigned int status_code;
  char *reason_phrase;

  Header headers[MAX_HEADERS];
  unsigned int headers_count;

  // Size of the headers and the response line
  unsigned int header_size;

  // Body
  unsigned char *body;
  unsigned int body_size;
} Response;

int parse_request(const unsigned char *raw, const long len, Request *req);

int parse_response(const unsigned char *raw, const long len, Response *res);

#endif /* PARSER_H */
