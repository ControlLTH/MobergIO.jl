#include <stdio.h>
#include <moberg.h>

int main(int argc, char *argv[])
{
  struct moberg *moberg = moberg_new(NULL);
  struct moberg_analog_in ai0;
  double ai0_value;
  if (! moberg_OK(moberg_analog_in_open(moberg, 0, &ai0))) {
    fprintf(stderr, "OPEN failed\n");
  }
  if (! moberg_OK(ai0.read(ai0.context, &ai0_value))) { 
    fprintf(stderr, "READ failed\n");
  }
  fprintf(stderr, "READ ai0: %f\n", ai0_value);
  moberg_analog_in_close(moberg, 0, ai0);
  moberg_free(moberg);
}
