/*
    moberg_device.c -- moberg device driver interface

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
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>
#include <moberg_channel.h>
#include <moberg_config.h>
#include <moberg_device.h>
#include <moberg_inline.h>

struct moberg_device {
  struct moberg_device_driver driver;
  struct moberg_device_context *device_context;
  struct channel_list {
    struct channel_list *next;
    enum moberg_channel_kind kind;
    int index;
    union channel {
      struct moberg_channel_analog_in *analog_in;
      struct moberg_channel_analog_out *analog_out;
      struct moberg_channel_digital_in *digital_in;
      struct moberg_channel_digital_out *digital_out;
      struct moberg_channel_encoder_in *encoder_in;
      struct moberg_channel *channel;
    } u;
  } *channel_head, **channel_tail;
  struct map_range {
    struct moberg_device *device;
    enum moberg_channel_kind kind;
    int min;
    int max;
  } *range;
};

struct moberg_device *moberg_device_new(struct moberg *moberg,
                                        const char *driver)
{
  struct moberg_device *result = NULL;

  char *name = malloc(strlen("libmoberg_.so") + strlen(driver) + 1);
  if (!name) { goto out; }
  sprintf(name, "libmoberg_%s.so", driver);
  void *handle = dlopen(name, RTLD_LAZY | RTLD_DEEPBIND);
  if (! handle) {
    fprintf(stderr, "Could not find driver %s %s\n", name, dlerror());
    goto free_name;
  }
  struct moberg_device_driver *device_driver =
    (struct moberg_device_driver *) dlsym(handle, "moberg_device_driver");
  if (! device_driver) {
    fprintf(stderr, "No moberg_device_driver in driver %s\n", name);
    goto dlclose_driver;
  }
  result = malloc(sizeof(*result));
  if (! result) {
    fprintf(stderr, "Could not allocate result for %s\n", name);
    goto dlclose_driver;
  }
  result->driver = *device_driver;
  result->device_context = result->driver.new(moberg, dlclose, handle);
  if (result->device_context) {
    result->driver.up(result->device_context);
  } else {
    fprintf(stderr, "Could not allocate context for %s\n", name);
    goto free_result;
  }
  result->channel_head = NULL;
  result->channel_tail = &result->channel_head;
  result->range = NULL;
  goto free_name;
  
free_result:
  free(result);
dlclose_driver:
  dlclose(handle);
free_name:
  free(name);
out:
  return result;
}

void moberg_device_free(struct moberg_device *device)
{
  struct channel_list *channel = device->channel_head;
  while (channel) {
    struct channel_list *next;
    next = channel->next;
    free(channel);
    channel = next;
  }
  device->driver.down(device->device_context);
  free(device);
}

int moberg_device_in_use(struct moberg_device *device)
{
  device->driver.up(device->device_context);
  int use = device->driver.down(device->device_context);
  return use > 1;
}

struct moberg_status moberg_device_parse_config(
  struct moberg_device *device,
  struct moberg_parser_context *parser)
{
  return device->driver.parse_config(device->device_context, parser);
}

static struct moberg_status add_channel(
  struct moberg_device* device,
  enum moberg_channel_kind kind,
  int index,
  union channel channel)
{
  struct channel_list *element = malloc(sizeof(*element));
  if (! element) { goto err; }
  element->next = NULL;
  element->kind = kind;
  element->index = index;
  element->u = channel;
  *device->channel_tail = element;
  device->channel_tail = &element->next;
  return MOBERG_OK;
err:
  return MOBERG_ERRNO(ENOMEM);
}

static struct moberg_status map(
  struct moberg_device* device,
  struct moberg_channel *channel)
{
  struct moberg_status result = MOBERG_ERRNO(EINVAL);

  if (device->range->kind != channel->kind) {
    fprintf(stderr, "Mapping wrong kind of channel %d != %d\n",
            device->range->kind, channel->kind);
    return MOBERG_ERRNO(EINVAL);
  }
  if (device->range->min > device->range->max) {
    fprintf(stderr, "Mapping outside range %d > %d\n",
            device->range->min, device->range->max);
    return MOBERG_ERRNO(ENOSPC);
  }
  result = add_channel(device, device->range->kind, device->range->min,
                       (union channel) { .channel=channel });
  if (! OK(result)) {
    fprintf(stderr, "Failed to add channel kind=%d, index=%d\n",
            device->range->kind, device->range->min);
    return result;
  }
  device->range->min++;
  return MOBERG_OK;
  
}

struct moberg_status moberg_device_parse_map(
  struct moberg_device* device,
  struct moberg_parser_context *parser,
  enum moberg_channel_kind kind,
  int min,
  int max)
{
  struct moberg_status result;
  struct map_range r = {
    .device=device,
    .kind=kind,
    .min=min,
    .max=max
  };
  struct moberg_channel_map map_channel = {
    .device=device,
    .map=map
  };
  
  device->range = &r;
  result = device->driver.parse_map(device->device_context, parser,
                                    kind, &map_channel);
  device->range = NULL;
  return result;
}

int moberg_device_install_channels(struct moberg_device *device,
                                   struct moberg_channel_install *install)
{
  
  struct channel_list *channel = device->channel_head;
  while (channel) {
    struct channel_list *next;
    next = channel->next;
    install->channel(install->context,
                     channel->index,
                     device,
                     channel->u.channel);
    channel = next;
  }
  return 1;
}

struct moberg_status moberg_device_start(struct moberg_device *device,
                                         FILE *f)
{
  return device->driver.start(device->device_context, f);
}

struct moberg_status moberg_device_stop(struct moberg_device *device,
                                        FILE *f)
{
  return device->driver.stop(device->device_context, f);
}
