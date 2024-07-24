#include <stdio.h>

typedef unsigned int uint;

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s url\n", *argv);
    return 1;
  }

  for (uint i = 1; i < argc; i++) {
    printf("%s\n", argv[i]);
  }

  return 0;
}
