/*
    serial2002_lib.h -- serial2002 protocol

    Copyright (C) 2002,2019 Anders Blomdell <anders.blomdell@gmail.com>

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

#ifndef __SERIAL2002_LIB_H__
#define __SERIAL2002_LIB_H__

#include <moberg.h>

struct serial2002_data {
  enum { is_invalid, is_digital, is_channel } kind;
  int index;
  unsigned long value;
};

enum serial2002_kind {
  SERIAL2002_DIGITAL_IN = 1,
  SERIAL2002_DIGITAL_OUT = 2,
  SERIAL2002_ANALOG_IN = 3,
  SERIAL2002_ANALOG_OUT = 4,
  SERIAL2002_COUNTER_IN = 5
};

struct serial2002_config {
  struct serial2002_channel {
    int valid;
    int kind;
    int bits;
    double min;
    double max;
  } channel_in[31], channel_out[31],
    digital_in[32], digital_out[32];
};

struct serial2002_io {
  int fd;
  struct buffer {
    unsigned char data[128];
    int pos;
    int count;
  } read, write;
};

struct moberg_status serial2002_poll_digital(struct serial2002_io *io,
                                             int channel,
                                             int flush);

struct moberg_status serial2002_poll_channel(struct serial2002_io *io,
                                             int channel,
                                             int flush);

struct moberg_status serial2002_read(struct serial2002_io *io,
                                     long timeout,
                                     struct serial2002_data *data);

struct moberg_status serial2002_write(struct serial2002_io *io,
                                      struct serial2002_data data,
                                      int flush);

struct moberg_status serial2002_read_config(struct serial2002_io *io,
                                            long timeout,
                                            struct serial2002_config *config);

struct moberg_status serial2002_flush(struct serial2002_io *io);

#endif
