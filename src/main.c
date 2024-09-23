#include <signal.h>

#include "common.h"
#include "proxy.h"

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
  switch (sig_num) {
  case 0:
    LOG(INFO, NULL, "Closing the proxy server...");
    break;
  default:
    LOG(INFO, NULL, "SIGINT Detected. Closing the proxy server...");
  }

  // cancel handler threads before, the proxy server
  for (int i = MAX_THREADS - 1; i >= 0; i--) {
    if (thread_pool[i] != 0)
      pthread_cancel(thread_pool[i]);
  }
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
    return EXIT_FAILURE;
  }

  char cmd[128] = {0};
  scanf("%127s", cmd);
  if (strncmp("exit", cmd, 4) == 0)
    stop_exec(0);

  pthread_join(thread_pool[0], NULL);
  return EXIT_SUCCESS;
}
