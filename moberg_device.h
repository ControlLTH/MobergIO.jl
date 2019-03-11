/*
    moberg_device.h -- moberg device driver interface

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
    
#ifndef __MOBERG_DEVICE_H__
#define __MOBERG_DEVICE_H__

struct moberg_device;
struct moberg_device_context;

#include <moberg.h>
#include <moberg_config.h>
#include <moberg_parser.h>
#include <moberg_channel.h>

struct moberg_parser_context;

struct moberg_device_driver {
  /* Create new device context */
  struct moberg_device_context *(*new)(struct moberg *moberg,
                                       int (*dlclose)(void *dlhandle),
                                       void *dlhandle);
  /* Use-count of device, when it reaches zero, device will be free'd */
  int (*up)(struct moberg_device_context *context);
  int (*down)(struct moberg_device_context *context);

  /* Parse driver dependent parts of config file */
  struct moberg_status (*parse_config)(
    struct moberg_device_context *device,
    struct moberg_parser_context *parser);
  struct moberg_status (*parse_map)(
    struct moberg_device_context *device,
    struct moberg_parser_context *parser,
    enum moberg_channel_kind kind,
    struct moberg_channel_map *map);

  /* Write shell commands for starting and stopping to FILE *f */
  struct moberg_status (*start)(
    struct moberg_device_context *device,
    FILE *f);
  struct moberg_status (*stop)(
    struct moberg_device_context *device,
    FILE *f);
  
};

struct moberg_device;

struct moberg_device *moberg_device_new(struct moberg *moberg,
                                        const char *driver);

void moberg_device_free(struct moberg_device *device);

int moberg_device_in_use(struct moberg_device *device);

struct moberg_status moberg_device_parse_config(
  struct moberg_device* device,
  struct moberg_parser_context *context);

struct moberg_status moberg_device_parse_map(
  struct moberg_device* device,
  struct moberg_parser_context *parser,
  enum moberg_channel_kind kind,
  int min,
  int max);

int moberg_device_install_channels(
  struct moberg_device *device,
  struct moberg_channel_install *install);

struct moberg_status moberg_device_start(
  struct moberg_device *device,
  FILE *f);

struct moberg_status moberg_device_stop(
  struct moberg_device *device,
  FILE *f);




#endif
