#include <stdio.h>
#include <stdlib.h>
#include <moberg.h>

int main(int argc, char *argv[])
{
  const struct moberg_t *moberg = moberg_init();
  free((struct moberg_t*) moberg);
}
