#include "common.h"

/* Thread pool related util func */
int find_empty_slot(void) {
  pthread_mutex_lock(&lock);
  if (thread_count >= MAX_THREADS) {
    pthread_mutex_unlock(&lock);
    return -1;
  }

  bool found = false;
  int i = 1; // thread_pool[0] is reserved for the proxy server
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

void remove_thread(pthread_t tid) {
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
    pthread_mutex_lock(&lock);
    exit(EXIT_FAILURE);
  }

  thread_count--;
  pthread_mutex_unlock(&lock);
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

/* Parser Utility Functions */
void free_req(Request *req) {
  // Free the request line
  free(req->method);
  free(req->uri);
  free(req->version);

  // Free the headers
  for (size_t i = 0; i < req->headers_count; i++) {
    free(req->headers[i].key);
    free(req->headers[i].value);
  }

  free(req->body); // Free the body
  free(req);       // Free the request struct
}

void free_res(Response *res) {
  // Free the request line
  free(res->version);
  free(res->status_code);
  free(res->reason_phrase);

  // Free the headers
  for (size_t i = 0; i < res->headers_count; i++) {
    free(res->headers[i].key);
    free(res->headers[i].value);
  }

  free(res->body); // Free the body
  free(res);       // Free the response struct
}

void print_req(Request *req) {
  printf(STYLE_DIM "\n####################################\n\n" STYLE_NO_DIM);

  printf(STYLE_BOLD "------ Request Line (Header Size: %u Bytes): \nMethod: "
                    "%s\nURI: %s\nVersion: %s\n\n",
         req->header_size, req->method, req->uri, req->version);

  printf("------ Headers: \n");
  for (size_t i = 0; i < req->headers_count; i++)
    printf("%s: %s\n", req->headers[i].key, req->headers[i].value);

  printf("\n------ Body (%u Bytes): \n" STYLE_NO_BOLD, req->body_size);
  if (req->body == NULL) {
    printf("No Body!\n");
  } else {
    unsigned char *body = (unsigned char *)malloc(req->body_size + 1);
    if (body == NULL) {
      LOG(ERR, NULL, "Failed to print request body");
      printf(STYLE_DIM
             "\n####################################\n\n" STYLE_NO_DIM);
      return;
    }
    memcpy(body, req->body, req->body_size);
    body[req->body_size] = '\0';
    printf("%s\n", body);
    free(body);
  }

  printf(STYLE_DIM "\n####################################\n\n" STYLE_NO_DIM);
}

void print_res(Response *res) {
  printf(STYLE_DIM "\n####################################\n\n" STYLE_NO_DIM);

  printf(STYLE_BOLD "------ Response Line (Header Size: %u Bytes): \nVersion: "
                    "%s\nStatus Code: %s\nReason Phrase: %s\n\n",
         res->header_size, res->version, res->status_code, res->reason_phrase);

  printf("------ Headers: \n");
  for (size_t i = 0; i < res->headers_count; i++)
    printf("%s: %s\n", res->headers[i].key, res->headers[i].value);

  printf("\n------ Body (%u Bytes): \n" STYLE_NO_BOLD, res->body_size);
  if (res->body == NULL) {
    printf("No Body!\n");
  } else {
    unsigned char *body = (unsigned char *)malloc(res->body_size + 1);
    if (body == NULL) {
      LOG(ERR, NULL, "Failed to print request body");
      printf(STYLE_DIM
             "\n####################################\n\n" STYLE_NO_DIM);
      return;
    }
    memcpy(body, res->body, res->body_size);
    body[res->body_size] = '\0';
    printf("%s\n", body);
    free(body);
  }

  printf(STYLE_DIM "\n####################################\n\n" STYLE_NO_DIM);
}

char *get_header_value(char *target, const Header *headers,
                       const size_t headers_count) {
  for (size_t i = 0; i < headers_count; i++)
    if (strncmp(target, headers[i].key, strlen(target)) == 0)
      return headers[i].value;

  return NULL;
}
