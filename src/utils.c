#include "common.h"

/* Thread pool related util func */
int find_empty_slot(void) {
  if (thread_count >= MAX_THREADS)
    return -1;

  bool found = false;
  int i = 1; // thread_pool[0] is reserved for the proxy server
  for (; i < MAX_THREADS; i++) {
    if (thread_pool[i] == 0) {
      found = true;
      break;
    }
  }

  if (!found)
    return -1;
  return i;
}

void remove_thread(pthread_t tid) {
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
    exit(EXIT_FAILURE);
  }

  thread_count--;
}

/* Recv/Forw */
int forward(const int dest_fd, const unsigned char *buffer, const long len) {
  long total_sent = 0;

  while (total_sent < len) {
    long bytes_send = send(dest_fd, buffer + total_sent, len - total_sent, 0);

    if (bytes_send == -1)
      return -1;

    total_sent += bytes_send;
  }

  return 0;
}

/* Parser Shit */
void free_req(Request *req) {
  if (req == NULL) {
    LOG(INFO, NULL, "Request empty. Nothing to free");
    return;
  }

  // Free the request line
  if (req->method != NULL)
    free(req->method);
  if (req->uri != NULL)
    free(req->uri);
  if (req->version != NULL)
    free(req->version);

  // Free the headers
  for (size_t i = 0; i < req->headers_count; i++) {
    if (req->headers[i].key != NULL)
      free(req->headers[i].key);
    if (req->headers[i].value != NULL)
      free(req->headers[i].value);
  }

  if (req->body != NULL)
    free(req->body); // Free the body
  free(req);         // Free the request struct
}

void print_req(Request *req) {
  printf(STYLE_DIM "\n####################################\n\n" STYLE_NO_DIM);

  printf(STYLE_BOLD "------ Request Line (Header Size: %zu Bytes): \nMethod: "
                    "%s\nURI: %s\nVersion: %s\n\n",
         req->header_size, req->method, req->uri, req->version);

  printf("------ Headers: \n");
  for (size_t i = 0; i < req->headers_count; i++)
    printf("%s: %s\n", req->headers[i].key, req->headers[i].value);

  printf("\n------ Body (%zu Bytes): \n%s\n" STYLE_NO_BOLD, req->body_size,
         req->body ? (char *)req->body : "No body");

  printf(STYLE_DIM "\n####################################\n\n" STYLE_NO_DIM);
}
