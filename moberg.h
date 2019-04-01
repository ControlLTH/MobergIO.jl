/*
    moberg.h -- interface to moberg I/O system

    Copyright (C) 2019 Anders Blomdell <anders.blomdell@gmail.com>

    This file is part of Moberg.

    Moberg is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
    
#ifndef __MOBERG_H__
#define __MOBERG_H__

#include <stdio.h>

struct moberg;

/* Error reporting */

struct moberg_status {
  int result; /* == 0 -> OK 
                 <  0 -> moberg specific error
                 >  0 -> system error (see errno.h */
};

int moberg_OK(struct moberg_status);

/* Creation & free */

struct moberg *moberg_new();

void moberg_free(struct moberg *moberg);

/* Input/output */

struct moberg_analog_in {
  struct moberg_channel_analog_in *context;
  struct moberg_status (*read)(struct moberg_channel_analog_in *,
                               double *value);
};

struct moberg_analog_out {
  struct moberg_channel_analog_out *context;
  struct moberg_status (*write)(struct moberg_channel_analog_out *,
                                double desired_value,
                                double *actual_value);
};

struct moberg_digital_in {
  struct moberg_channel_digital_in *context;
  struct moberg_status (*read)(struct moberg_channel_digital_in *,
                               int *value);
};

struct moberg_digital_out {
  struct moberg_channel_digital_out *context;
  struct moberg_status (*write)(struct moberg_channel_digital_out *,
                                int desired_value,
                                int *actual_value);
};

struct moberg_encoder_in {
  struct moberg_channel_encoder_in *context;
  struct moberg_status (*read)(struct moberg_channel_encoder_in *,
                               long *value);
};

struct moberg_status moberg_analog_in_open(
  struct moberg *moberg,
  int index,
  struct moberg_analog_in *analog_in);

struct moberg_status moberg_analog_in_close(
  struct moberg *moberg,
  int index,
  struct moberg_analog_in analog_in);

struct moberg_status moberg_analog_out_open(
  struct moberg *moberg,
  int index,
  struct moberg_analog_out *analog_out);

struct moberg_status moberg_analog_out_close(
  struct moberg *moberg,
  int index,
  struct moberg_analog_out analog_out);

struct moberg_status moberg_digital_in_open(
  struct moberg *moberg,
  int index,
  struct moberg_digital_in *digital_in);

struct moberg_status moberg_digital_in_close(
  struct moberg *moberg,
  int index,
  struct moberg_digital_in digital_in);

struct moberg_status moberg_digital_out_open(
  struct moberg *moberg,
  int index,
  struct moberg_digital_out *digital_out);

struct moberg_status moberg_digital_out_close(
  struct moberg *moberg,
  int index,
  struct moberg_digital_out digital_out);

struct moberg_status moberg_encoder_in_open(
  struct moberg *moberg,
  int index,
  struct moberg_encoder_in *encoder_in);

struct moberg_status moberg_encoder_in_close(
  struct moberg *moberg,
  int index,
  struct moberg_encoder_in encoder_in);

/* System init functionality (systemd/init/...) */

struct moberg_status moberg_start(
  struct moberg *moberg,
  FILE *f);

struct moberg_status moberg_stop(
  struct moberg *moberg,
  FILE *f);

#endif
