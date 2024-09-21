#include "proxy.h"

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

int main(int argc, char **argv) {
  print_banner();
  if (argc != 2) {
    LOG(INFO, NULL, "USAGE: %s PORT", argv[0]);
    return EXIT_FAILURE;
  }

  pthread_t proxy_tid = 0;
  if (pthread_create(&proxy_tid, NULL, proxy, argv[1]) != 0) {
    LOG(ERR, NULL, "Failed to create proxy server thread");
    return EXIT_FAILURE;
  }

  pthread_join(proxy_tid, NULL);
  return EXIT_SUCCESS;
}
