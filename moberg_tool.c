#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <moberg.h>

void usage(char *prog) {
  fprintf(stderr, "%s [ --start | --stop | -h | --help ]\n", prog);
}

int main(int argc, char *argv[])
{
  
  if (argc == 2 && strcmp(argv[1], "--start") == 0) {
    struct moberg *moberg = moberg_new(NULL);
    moberg_start(moberg, stdout);
    moberg_free(moberg);    
  } else if (argc == 2 && strcmp(argv[1], "--stop") == 0) {
    struct moberg *moberg = moberg_new(NULL);
    moberg_stop(moberg, stdout);
    moberg_free(moberg);    
  } else if (argc == 2 && strcmp(argv[1], "-h") == 0) {
    usage(argv[0]);
  } else if (argc == 2 && strcmp(argv[1], "--help") == 0) {
    usage(argv[0]);
  } else {
    usage(argv[0]);
    exit(1);
  }
}
