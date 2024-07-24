#include <stdio.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("%s url\n", *argv);
    return 1;
  }

  printf("Pinging %s!\n", argv[1]);

  return 0;
}
