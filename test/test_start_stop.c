#include <stdio.h>
#include <moberg.h>

int main(int argc, char *argv[])
{
  fprintf(stderr, "NEW\n");
  struct moberg *moberg = moberg_new(NULL);
  fprintf(stderr, "START:\n");
  moberg_start(moberg, stdout);
  fprintf(stderr, "STOP:\n");
  moberg_stop(moberg, stdout);
  fprintf(stderr, "FREE\n");
  moberg_free(moberg);
  fprintf(stderr, "DONE\n");
}
