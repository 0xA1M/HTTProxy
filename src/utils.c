#include "common.h"

/*****************************************************
 *            Thread Management Functions            *
 *****************************************************/
int find_empty_slot(void) {
  pthread_mutex_lock(&lock);
  if (thread_count >= MAX_THREADS) {
    pthread_mutex_unlock(&lock);
    return -1;
  }

  bool found = false;
  int i = 1; // PROXY_TID_INDEX=0 is reserved for the proxy server tid
  for (; i < MAX_THREADS; i++) {
    if (thread_pool[i] == 0) {
      found = true;
      break;
    }
  }
  pthread_mutex_unlock(&lock);

  if (!found)
    return -1;
  return i;
}

void remove_thread(const pthread_t tid) {
  pthread_mutex_lock(&lock);
  bool found = false;

  for (int i = 0; i < MAX_THREADS; i++) {
    if (tid == thread_pool[i]) {
      thread_pool[i] = 0;
      found = true;
      break;
    }
  }

  if (!found) {
    LOG(ERR, NULL, "Thread ID isn't registered!");
    pthread_mutex_unlock(&lock);
    exit(EXIT_FAILURE);
  }

  if (thread_count > 0)
    thread_count--;
  pthread_mutex_unlock(&lock);
}

/*********************************************************
 *            Connection Management Functions            *
 *********************************************************/
int forward(const int dest_fd, const unsigned char *buffer, const long len) {
  ssize_t total_sent = 0;

  while (total_sent < len) {
    ssize_t bytes_send =
        send(dest_fd, buffer + total_sent, len - total_sent, 0);

    // Connection closed
    if (bytes_send == 0)
      return -1;

    // Check for errors during sending
    if (bytes_send == -1) {
      // if errno is EINTR (interrupted system call) or EAGAIN (non-blocking
      // socket would block), retry
      if (errno == EINTR || errno == EAGAIN)
        continue;

      return -1;
    }

    total_sent += bytes_send;
  }

  return 0;
}

/*****************************************************
 *            Memory Management Functions            *
 *****************************************************/
void free_req(Request **req) {
  if (req == NULL || *req == NULL)
    return;

  // Free the request line
  free((*req)->method);
  (*req)->method = NULL;
  free((*req)->uri);
  (*req)->uri = NULL;
  free((*req)->version);
  (*req)->version = NULL;

  // Free the headers
  for (size_t i = 0; i < (*req)->headers_count; i++) {
    free((*req)->headers[i].key);
    (*req)->headers[i].key = NULL;
    free((*req)->headers[i].value);
    (*req)->headers[i].value = NULL;
  }

  // Free the body and the struct itself
  free((*req)->body);
  (*req)->body = NULL;
  free(*req);
  *req = NULL;
}

void free_res(Response **res) {
  if (res == NULL || *res == NULL)
    return;

  // Free the request line
  free((*res)->version);
  (*res)->version = NULL;
  free((*res)->status_code);
  (*res)->status_code = NULL;
  free((*res)->reason_phrase);
  (*res)->reason_phrase = NULL;

  // Free the headers
  for (size_t i = 0; i < (*res)->headers_count; i++) {
    free((*res)->headers[i].key);
    (*res)->headers[i].key = NULL;
    free((*res)->headers[i].value);
    (*res)->headers[i].value = NULL;
  }

  // Free the body and the struct itself
  free((*res)->body);
  (*res)->body = NULL;
  free(*res);
  *res = NULL;
}

/*********************************************************
 *            HTTP Message Printing Functions            *
 *********************************************************/
static void print_body(const unsigned char *raw, const size_t body_size) {
  if (raw == NULL || body_size == 0) {
    printf(STYLE_BOLD "No Body!\n" STYLE_NO_BOLD);
    return;
  }

  unsigned char *body = (unsigned char *)malloc(body_size + 1);
  if (body == NULL) {
    LOG(ERR, NULL, "Failed to print request body");
    printf(STYLE_DIM "\n####################################\n\n" STYLE_NO_DIM);
    return;
  }
  memset(body, 0, body_size);

  memcpy(body, raw, body_size);
  body[body_size] = '\0';

  printf("%.*s\n", (int)body_size, body);

  free(body);
  body = NULL;
}

void print_req(const Request *req) {
  if (req == NULL) {
    LOG(ERR, NULL, "NULL request pointer provided");
    return;
  }

  printf(STYLE_DIM "\n####################################\n\n" STYLE_NO_DIM);

  printf(STYLE_BOLD "------ Request Line (Header Size: %zu Bytes): \nMethod: "
                    "%s\nURI: %s\nVersion: %s\n\n",
         req->header_size, req->method, req->uri, req->version);

  printf("------ Headers: \n");
  for (size_t i = 0; i < req->headers_count; i++)
    printf("%s: %s\n", req->headers[i].key, req->headers[i].value);

  printf("\n------ Body (Body Size: %zu Bytes%s): \n" STYLE_NO_BOLD,
         req->body_size, req->is_chunked ? ", chunked" : "");
  if ((req->is_text || req->content_type == NULL) &&
      req->content_encoding == NULL)
    print_body(req->body, req->body_size);
  else
    print_hex(req->body, req->body_size);

  printf(STYLE_DIM "\n####################################\n\n" STYLE_NO_DIM);
}

void print_res(const Response *res) {
  if (res == NULL) {
    LOG(ERR, NULL, "NULL response pointer provided");
    return;
  }

  printf(STYLE_DIM "\n####################################\n\n" STYLE_NO_DIM);

  printf(STYLE_BOLD "------ Response Line (Header Size: %zu Bytes): \nVersion: "
                    "%s\nStatus Code: %s\nReason Phrase: %s\n\n",
         res->header_size, res->version, res->status_code, res->reason_phrase);

  printf("------ Headers: \n");
  for (size_t i = 0; i < res->headers_count; i++)
    printf("%s: %s\n", res->headers[i].key, res->headers[i].value);

  printf("\n------ Body (Body Size: %zu Bytes%s): \n" STYLE_NO_BOLD,
         res->body_size, res->is_chunked ? ", chunked" : "");

  if ((res->is_text || res->content_type == NULL) &&
      res->content_encoding == NULL)
    print_body(res->body, res->body_size);
  else
    print_hex(res->body, res->body_size);

  printf(STYLE_DIM "\n####################################\n\n" STYLE_NO_DIM);
}

void print_hex(const unsigned char *buffer, const size_t buffer_len) {
  if (buffer == NULL || buffer_len == 0)
    return;

  if (buffer == NULL && buffer_len > 0) {
    LOG(ERR, NULL, "NULL buffer provided with non-zero length");
    return;
  }

  for (size_t i = 0; i < buffer_len; i++) {
    if (i != 0 && i % 32 == 0)
      printf("\n");

    printf("%.2X ", buffer[i]);
  }

  printf("\n");
}

/********************************************************
 *            HTTP Message Parsing Functions            *
 ********************************************************/
const char *get_header_value(const char *target, const Header *headers,
                             const size_t headers_count) {
  if (target == NULL || headers == NULL)
    return NULL;

  size_t target_len = strlen(target);
  if (target_len == 0)
    return NULL;

  for (size_t i = 0; i < headers_count; i++) {
    if (headers[i].key == NULL)
      continue;

    if (strncasecmp(target, headers[i].key, target_len) == 0)
      return headers[i].value;
  }

  return NULL;
}

int get_chunk_size(const unsigned char *chunk, const size_t chunk_len) {
  if (chunk == NULL || chunk_len == 0) {
    LOG(WARN, NULL, "Invalid chunk input");
    return -1;
  }

  const unsigned char *size_str_end = memmem(chunk, chunk_len, "\r\n", 2);
  if (size_str_end == NULL) {
    LOG(WARN, NULL, "Invalid chunk format!");
    return -1;
  }

  const size_t size_str_len = size_str_end - chunk;
  if (size_str_len <= 0) {
    LOG(WARN, NULL, "Invalid chunk format!");
    return -1;
  }

  // Reasonable maximum length for a hex size string
  if (size_str_len > 16) {
    LOG(WARN, NULL, "Chunk size string too long");
    return -1;
  }

  // Use stack allocation for small string
  char size_str[17]; // 16 hex digits + null terminator
  memcpy(size_str, chunk, size_str_len);
  size_str[size_str_len] = '\0';

  // Convert hex string to number
  char *endptr = NULL;
  const int chunk_size = strtol(size_str, &endptr, 16);

  // Validate conversion
  if (endptr == size_str || *endptr != '\0' || chunk_size < 0) {
    LOG(WARN, NULL, "Invalid chunk size format");
    return -1;
  }

  return chunk_size;
}
