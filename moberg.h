#ifndef __MOBERG_H__
#define __MOBERG_H__

#include <stdio.h>

struct moberg;
struct moberg_config;

/* Creation & free */

struct moberg *moberg_new(struct moberg_config *config);

void moberg_free(struct moberg *moberg);

/* Input/output */

struct moberg_analog_in {
  struct moberg_channel_analog_in *context;
  int (*read)(struct moberg_channel_analog_in *, double *value);
};

struct moberg_analog_out {
  struct moberg_channel_analog_out *context;
  int (*write)(struct moberg_channel_analog_out *, double value);
};

struct moberg_digital_in {
  struct moberg_channel_digital_in *context;
  int (*read)(struct moberg_channel_digital_in *, int *value);
};

struct moberg_digital_out {
  struct moberg_channel_digital_out *context;
  int (*write)(struct moberg_channel_digital_out *, int value);
};

struct moberg_encoder_in {
  struct moberg_channel_encoder_in *context;
  int (*read)(struct moberg_channel_encoder_in *, long *value);
};

int moberg_analog_in_open(struct moberg *moberg,
                          int index,
                          struct moberg_analog_in *analog_in);

int moberg_analog_in_close(struct moberg *moberg,
                           int index,
                           struct moberg_analog_in analog_in);

/* System init functionality (systemd/init/...) */

int moberg_start(
  struct moberg *moberg,
  FILE *f);

int moberg_stop(
  struct moberg *moberg,
  FILE *f);

#endif
