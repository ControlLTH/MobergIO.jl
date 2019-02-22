#include <stdio.h>
#include <moberg.h>

int main(int argc, char *argv[])
{
  struct moberg *moberg = moberg_new(NULL);
  printf("START:\n");
  moberg_start(moberg, stdout);
  printf("STOP:\n");
  moberg_stop(moberg, stdout);
  printf("DONE\n");
  moberg_free(moberg);
}
