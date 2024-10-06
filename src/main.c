#include <signal.h>
#include <unistd.h>

#include "common.h"
#include "proxy.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t thread_pool[MAX_THREADS] = {0};
int thread_count = 0;

static void print_banner(void) {
  printf("$$\\   $$\\ $$$$$$$$\\ $$$$$$$$\\ $$$$$$$\\\n");
  printf("$$ |  $$ |\\__$$  __|\\__$$  __|$$  __$$\\\n");
  printf("$$ |  $$ |   $$ |      $$ |   $$ |  $$ | $$$$$$\\   $$$$$$\\  $$\\   "
         "$$\\ $$\\   $$\\\n");
  printf("$$$$$$$$ |   $$ |      $$ |   $$$$$$$  |$$  __$$\\ $$  __$$\\ \\$$\\ "
         "$$  "
         "|$$ |  $$ |\n");
  printf(
      "$$  __$$ |   $$ |      $$ |   $$  ____/ $$ |  \\__|$$ /  $$ | \\$$$$  "
      "/ $$ |  $$ |\n");
  printf("$$ |  $$ |   $$ |      $$ |   $$ |      $$ |      $$ |  $$ | $$  $$< "
         " $$ |  $$ |\n");
  printf("$$ |  $$ |   $$ |      $$ |   $$ |      $$ |      \\$$$$$$  |$$  "
         "/\\$$\\ \\$$$$$$$ |\n");
  printf("\\__|  \\__|   \\__|      \\__|   \\__|      \\__|       \\______/ "
         "\\__/  "
         "\\__| \\____$$ |\n");
  printf("                                                                     "
         " $$\\   $$ |\n");
  printf("                                                                     "
         " \\$$$$$$  |\n");
  printf("                                                                     "
         "  \\______/\n");
}

static void stop_exec(int sig_num) {
  (void)sig_num;
  const char *msg = "Closing the proxy server...";
  write(1, STYLE_BOLD, strlen(STYLE_BOLD));
  write(1, MSG_COLOR, strlen(MSG_COLOR));
  write(1, msg, strlen(msg));
  write(1, RESET, strlen(RESET));

  // cancel handler threads before, the proxy server
  pthread_mutex_lock(&lock);
  for (int i = MAX_THREADS - 1; i >= 0; i--) {
    if (thread_pool[i] != 0)
      pthread_cancel(thread_pool[i]);
  }
  pthread_mutex_unlock(&lock);
  pthread_mutex_destroy(&lock);
}

static void init_sig_handler(void) {
  struct sigaction sa;

  sa.sa_handler = stop_exec;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &sa, NULL) == -1) {
    LOG(ERR, NULL, "Failed to initialize signal handler");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv) {
  print_banner();
  if (argc != 2) {
    LOG(INFO, NULL, "USAGE: %s PORT", argv[0]);
    return EXIT_FAILURE;
  }

  init_sig_handler();

  if (pthread_create(&thread_pool[thread_count++], NULL, proxy, argv[1]) != 0) {
    LOG(ERR, NULL, "Failed to create proxy server thread");
    pthread_mutex_destroy(&lock);
    return EXIT_FAILURE;
  }

  pthread_join(thread_pool[0], NULL);
  return EXIT_SUCCESS;
}
