#include <stdio.h>
#include <moberg.h>

int main(int argc, char *argv[])
{
  struct moberg *moberg = moberg_new(NULL);
  if (! moberg) {
    fprintf(stderr, "NEW failed\n");
    goto out;
  }
  struct moberg_analog_in ai0;
  struct moberg_analog_out ao0;
  double ai0_value, ao0_actual;
  if (! moberg_OK(moberg_analog_in_open(moberg, 0, &ai0))) {
    fprintf(stderr, "OPEN failed\n");
    goto free;
  } 
  if (! moberg_OK(ai0.read(ai0.context, &ai0_value))) { 
    fprintf(stderr, "READ failed\n");
    goto close_ai0;
  }
  fprintf(stderr, "READ ai0: %f\n", ai0_value);
  if (! moberg_OK(moberg_analog_out_open(moberg, 0, &ao0))) {
    fprintf(stderr, "OPEN failed\n");
    goto free;
  } 
  if (! moberg_OK(ao0.write(ao0.context, ai0_value * 2, &ao0_actual))) { 
    fprintf(stderr, "READ failed\n");
    goto close_ao0;
  }
  fprintf(stderr, "WROTE ao0: %f %f\n", ai0_value * 2, ao0_actual);
close_ao0:
  moberg_analog_out_close(moberg, 0, ao0);
close_ai0:
  moberg_analog_in_close(moberg, 0, ai0);
free:
  moberg_free(moberg);
out:
  return 0;
}
