#include <moberg.h>

int main(int argc, char *argv)
{

  struct moberg *moberg = moberg_new();
  
  for (int i = 0 ; i < 500 ; i++) {
    struct moberg_analog_in analog_in;

    struct moberg_status status;
    double value;
    status = moberg_analog_in_open(moberg, i, &analog_in);
    if (moberg_OK(status)) {
      status = analog_in.read(analog_in.context, &value);
      if (moberg_OK(status)) {
        printf("%03d: %f\n", i, value);
      }
    }
    status = moberg_analog_in_close(moberg, i, analog_in);
  }
  moberg_free(moberg);
}
