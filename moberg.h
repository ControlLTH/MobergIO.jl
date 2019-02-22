#ifndef __MOBERG_H__
#define __MOBERG_H__

#include <stdio.h>

struct moberg;
struct moberg_config;

enum moberg_status { moberg_OK };

struct moberg *moberg_new(struct moberg_config *config);

void moberg_free(struct moberg *moberg);

/* Input/output functions */

enum moberg_status moberg_analog_in(
  double *value,
  struct moberg *moberg,
  int channel);

enum moberg_status moberg_analog_out(
  double value,
  struct moberg *moberg,
  int channel);

enum moberg_status moberg_digital_in(
  int *value,
  struct moberg *moberg,
  int channel);

enum moberg_status moberg_digital_out(
  int value,
  struct moberg *moberg,
  int channel);

enum moberg_status moberg_encoder_in(
  long *value,
  struct moberg *moberg,
  int channel);

/* Install functionality */

enum moberg_status moberg_start(
  struct moberg *moberg,
  FILE *f);

enum moberg_status moberg_stop(
  struct moberg *moberg,
  FILE *f);

#endif
