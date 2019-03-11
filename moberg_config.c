/*
    moberg_config.c -- moberg configuration handling

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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <moberg.h>
#include <moberg_config.h>
#include <moberg_inline.h>

struct moberg_config
{
  struct device_entry {
    struct device_entry *next;
    struct moberg_device *device;
  } *device_head, **device_tail;
};

struct moberg_config *moberg_config_new()
{
  struct moberg_config *result = malloc(sizeof *result);

  if (result) {
    result->device_head = NULL;
    result->device_tail = &result->device_head;
  }
  
  return result;
}

void moberg_config_free(struct moberg_config *config)
{
  if (config) {
    struct device_entry *entry = config->device_head;
    while (entry) {
      struct device_entry *tmp = entry;
      entry = entry->next;
      moberg_device_free(tmp->device);
      free(tmp);
    }
    free(config);
  }
}

int moberg_config_join(struct moberg_config *dest,
                       struct moberg_config *src)
{
  if (src && dest) {
    while (src->device_head) {
      struct device_entry *d = src->device_head;
      src->device_head = d->next;
      if (! src->device_head) {
        src->device_tail = &src->device_head;
      }
        
      *dest->device_tail = d;
      dest->device_tail = &d->next;
    }
    return 1;
  }
  return 0;
}

struct moberg_status moberg_config_add_device(struct moberg_config *config,
                                              struct moberg_device *device)
{
  struct device_entry *entry = malloc(sizeof(*entry));

  if (! entry) { goto err; }
  entry->next = NULL;
  entry->device = device;
  /* TODO_ entry->started = 0; */
  *config->device_tail = entry;
  config->device_tail = &entry->next;

  return MOBERG_OK;
err:
  return MOBERG_ERRNO(ENOMEM);
}

int moberg_config_install_channels(struct moberg_config *config,
                                   struct moberg_channel_install *install)
{
  int result = 1;
  for (struct device_entry *d = config->device_head ; d ; d = d->next) {
    result &= moberg_device_install_channels(d->device, install);
  }
  struct device_entry *device = config->device_head;
  struct device_entry **previous = &config->device_head;
  while (device) {
    struct device_entry *next = device->next;
    if (moberg_device_in_use(device->device)) {
      previous = &device->next;
    } else {
      moberg_device_free(device->device);
      free(device);
      *previous = next;
    }
    device = next;
  }

  return result;
}

struct moberg_status moberg_config_start(struct moberg_config *config,
                                         FILE *f)
{
  for (struct device_entry *d = config->device_head ; d ; d = d->next) {
    moberg_device_start(d->device, f);
  }
  return MOBERG_OK;
}


struct moberg_status moberg_config_stop(struct moberg_config *config,
                                        FILE *f)
{
  for (struct device_entry *d = config->device_head ; d ; d = d->next) {
    moberg_device_stop(d->device, f);
  }
  return MOBERG_OK;
}
