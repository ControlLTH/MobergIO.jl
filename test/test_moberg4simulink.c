#include <moberg4simulink.h>

int main(int argc, char *argv[])
{
  struct moberg_analog_in *ain = moberg4simulink_analog_in_open(0);
  if (!ain) {
    fprintf(stderr, "OPEN failed\n");
    goto out;
  }
  moberg4simulink_analog_in_close(0, ain);
  return(0);
 out:
  return 1;
}
