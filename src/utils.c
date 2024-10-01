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
