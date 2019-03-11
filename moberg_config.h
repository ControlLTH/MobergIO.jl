/*
    moberg_config.h -- moberg configuration handling

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

#ifndef __MOBERG_CONFIG_H__
#define __MOBERG_CONFIG_H__

#include <moberg.h>
#include <moberg_device.h>

struct moberg_config *moberg_config_new();

void moberg_config_free(struct moberg_config *config);

int moberg_config_join(struct moberg_config *dest,
                       struct moberg_config *src);

struct moberg_status moberg_config_add_device(struct moberg_config *config,
                                              struct moberg_device *device);

int moberg_config_install_channels(struct moberg_config *config,
                                   struct moberg_channel_install *install);

struct moberg_status moberg_config_start(struct moberg_config *config,
                                         FILE *f);

struct moberg_status moberg_config_stop(struct moberg_config *config,
                                        FILE *f);

#endif
