/*
    moberg_channel.h -- moberg channel interface

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
    
#ifndef __MOBERG_CHANNEL_H__
#define __MOBERG_CHANNEL_H__

#include <moberg.h>

enum moberg_channel_kind {
    chan_ANALOGIN,
    chan_ANALOGOUT,
    chan_DIGITALIN,
    chan_DIGITALOUT,
    chan_ENCODERIN
};

struct moberg_channel {
  struct moberg_channel_context *context;
  
  /* Use-count of channel, when it reaches zero, channel will be free'd */
  int (*up)(struct moberg_channel *channel);
  int (*down)(struct moberg_channel *channel);

  /* Channel open and close */
  struct moberg_status (*open)(struct moberg_channel *channel);
  struct moberg_status (*close)(struct moberg_channel *channel);

  /* I/O operations */
  enum moberg_channel_kind kind;
  union moberg_channel_action {
    struct moberg_analog_in analog_in;
    struct moberg_analog_out analog_out;
    struct moberg_digital_in digital_in;
    struct moberg_digital_out digital_out;
    struct moberg_encoder_in encoder_in;
  } action;
};
  
struct moberg_channel_map {
  struct moberg_device *device;
  struct moberg_status (*map)(struct moberg_device* device,
                              struct moberg_channel *channel);
};

struct moberg_channel_install {
  struct moberg *context;
  struct moberg_status (*channel)(struct moberg *context,
                                  int index,
                                  struct moberg_device* device,
                                  struct moberg_channel *channel);
};

#endif
