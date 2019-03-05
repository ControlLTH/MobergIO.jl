#include <moberg4simulink.h>

int main(int argc, char *argv[])
{
  struct moberg_analog_in *ain = moberg4simulink_analog_in_open(0);
  moberg4simulink_analog_in_close(0, ain);
}
