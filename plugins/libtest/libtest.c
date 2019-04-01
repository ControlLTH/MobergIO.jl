/*
    libtest.c -- libtest plugin for moberg

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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/termbits.h>
#include <linux/serial.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <moberg.h>
#include <moberg_config.h>
#include <moberg_device.h>
#include <moberg_inline.h>
#include <moberg_module.h>
#include <moberg_parser.h>

struct moberg_device_context {
  struct moberg *moberg;
  int (*dlclose)(void *dlhandle);
  void *dlhandle;
  int use_count;
  double analog;
  int digital;
  long encoder;
};

struct moberg_channel_context {
  void *to_free; /* To be free'd when use_count goes to zero */
  struct moberg_device_context *device;
  int use_count;
  int index;
};

struct moberg_channel_analog_in {
  struct moberg_channel channel;
  struct moberg_channel_context channel_context;
};

struct moberg_channel_analog_out {
  struct moberg_channel channel;
  struct moberg_channel_context channel_context;
};

struct moberg_channel_digital_in {
  struct moberg_channel channel;
  struct moberg_channel_context channel_context;
};

struct moberg_channel_digital_out {
  struct moberg_channel channel;
  struct moberg_channel_context channel_context;
};

struct moberg_channel_encoder_in {
  struct moberg_channel channel;
  struct moberg_channel_context channel_context;
};

static struct moberg_status analog_in_read(
  struct moberg_channel_analog_in *analog_in,
  double *value)
{
  if (! value) { goto err_einval; }
  
  struct moberg_channel_context *channel = &analog_in->channel_context;
  struct moberg_device_context *device = channel->device;
  *value = device->analog / (channel->index + 1);
  return MOBERG_OK;
err_einval:
  return MOBERG_ERRNO(EINVAL);

}

static struct moberg_status analog_out_write(
  struct moberg_channel_analog_out *analog_out,
  double desired_value,
  double *actual_value)
{
  struct moberg_channel_context *channel = &analog_out->channel_context;
  struct moberg_device_context *device = channel->device;
  device->analog = desired_value * (channel->index + 1);
  if (actual_value) {
    *actual_value = desired_value;
  }
  return MOBERG_OK;
}

static struct moberg_status digital_in_read(
  struct moberg_channel_digital_in *digital_in,
  int *value)
{
  if (! value) { goto err_einval; }

  struct moberg_channel_context *channel = &digital_in->channel_context;
  struct moberg_device_context *device = channel->device;
  *value = (device->digital & (1<<channel->index)) != 0;
  return MOBERG_OK;
err_einval:
  return MOBERG_ERRNO(EINVAL);
}

static struct moberg_status digital_out_write(
  struct moberg_channel_digital_out *digital_out,
  int desired_value,
  int *actual_value)
{
  struct moberg_channel_context *channel = &digital_out->channel_context;
  struct moberg_device_context *device = channel->device;
  int mask =  (1<<channel->index);
  if (desired_value) {
    device->digital |= mask;
  } else {
    device->digital &= ~mask;
  }
  if (actual_value) {
    *actual_value = desired_value;
  }
  return MOBERG_OK;
}

static struct moberg_status encoder_in_read(
  struct moberg_channel_encoder_in *encoder_in,
  long *value)
{
  if (! value) { goto err_einval; }
  
  struct moberg_channel_context *channel = &encoder_in->channel_context;
  struct moberg_device_context *device = channel->device;
  *value = device->digital * (channel->index + 1);
  return MOBERG_OK;
err_einval:
  return MOBERG_ERRNO(EINVAL);
}

static struct moberg_device_context *new_context(struct moberg *moberg,
                                                 int (*dlclose)(void *dlhandle),
                                                 void *dlhandle)
{
  struct moberg_device_context *result = malloc(sizeof(*result));
  if (result) {
    memset(result, 0, sizeof(*result));
    result->moberg = moberg;
    result->dlclose = dlclose;
    result->dlhandle = dlhandle;
  }
  return result;
}

static int device_up(struct moberg_device_context *context)
{
  context->use_count++;
  return context->use_count;
}

static int device_down(struct moberg_device_context *context)
{
  context->use_count--;
  if (context->use_count <= 0) {
    moberg_deferred_action(context->moberg,
                           context->dlclose, context->dlhandle);
    free(context);
    return 0;
  }
  return context->use_count;
}

static struct moberg_status device_open(struct moberg_device_context *device)
{
  return MOBERG_OK;
}

static struct moberg_status device_close(struct moberg_device_context *device)
{
  return MOBERG_OK;
}

static int channel_up(struct moberg_channel *channel)
{
  device_up(channel->context->device);
  channel->context->use_count++;
  return channel->context->use_count;
}

static int channel_down(struct moberg_channel *channel)
{
  device_down(channel->context->device);
  channel->context->use_count--;
  if (channel->context->use_count <= 0) {
    free(channel->context->to_free);
    return 0;
  }
  return channel->context->use_count;
}

static struct moberg_status channel_open(struct moberg_channel *channel)
{
  device_open(channel->context->device);
  return MOBERG_OK;
}

static struct moberg_status channel_close(struct moberg_channel *channel)
{
  device_close(channel->context->device);
  return MOBERG_OK;
}

static void init_channel(
  struct moberg_channel *channel,
  void *to_free,
  struct moberg_channel_context *context,
  struct moberg_device_context *device,
  int index,
  enum moberg_channel_kind kind,
  union moberg_channel_action action)
{
  context->to_free = to_free;
  context->device = device;
  context->use_count = 0;
  context->index = index;
  
  channel->context = context;
  channel->up = channel_up;
  channel->down = channel_down;
  channel->open = channel_open;
  channel->close = channel_close;
  channel->kind = kind;
  channel->action = action;
};

static struct moberg_status parse_config(
  struct moberg_device_context *device,
  struct moberg_parser_context *c)
{
  if (! acceptsym(c, tok_LBRACE, NULL)) { goto syntax_err; }
  for (;;) {
    if (acceptsym(c, tok_RBRACE, NULL)) {
	break;
    } else {
      goto syntax_err;
    }
  }
  return MOBERG_OK;
syntax_err:
  return moberg_parser_failed(c, stderr);
}

static struct moberg_status parse_map(
  struct moberg_device_context *device,
  struct moberg_parser_context *c,
  enum moberg_channel_kind ignore,
  struct moberg_channel_map *map)
{
  enum moberg_channel_kind kind;
  token_t min, max;
 
  if (acceptkeyword(c, "analog_in")) { kind = chan_ANALOGIN; }
  else if (acceptkeyword(c, "analog_out")) { kind = chan_ANALOGOUT; }
  else if (acceptkeyword(c, "digital_in")) { kind = chan_DIGITALIN; }
  else if (acceptkeyword(c, "digital_out")) { kind = chan_DIGITALOUT; }
  else if (acceptkeyword(c, "encoder_in")) { kind = chan_ENCODERIN; }
  else { goto syntax_err; }
  if (! acceptsym(c, tok_LBRACKET, NULL)) { goto syntax_err; }
  if (! acceptsym(c, tok_INTEGER, &min)) { goto syntax_err; }
  if (acceptsym(c, tok_COLON, NULL)) { 
    if (! acceptsym(c, tok_INTEGER, &max)) { goto syntax_err; }
  } else {
    max = min;
  }
  if (! acceptsym(c, tok_RBRACKET, NULL)) { goto syntax_err; }
  for (int i = min.u.integer.value ; i <= max.u.integer.value ; i++) {
    switch (kind) {
      case chan_ANALOGIN: {
        struct moberg_channel_analog_in *channel = malloc(sizeof(*channel));

        if (! channel) { goto err_enomem; }
        init_channel(&channel->channel,
                     channel,
                     &channel->channel_context,
                     device,
                     i,
                     kind,
                     (union moberg_channel_action) {
                       .analog_in.context=channel,
                       .analog_in.read=analog_in_read });
        map->map(map->device, &channel->channel);
      } break;
      case chan_ANALOGOUT: {
        struct moberg_channel_analog_out *channel = malloc(sizeof(*channel));
        
        if (! channel) { goto err_enomem; }
        init_channel(&channel->channel,
                     channel,
                     &channel->channel_context,
                     device,
                     i,
                     kind,
                     (union moberg_channel_action) {
                       .analog_out.context=channel,
                       .analog_out.write=analog_out_write });
        map->map(map->device, &channel->channel);
      } break;
      case chan_DIGITALIN: {
        struct moberg_channel_digital_in *channel = malloc(sizeof(*channel));

        if (! channel) { goto err_enomem; }
        init_channel(&channel->channel,
                     channel,
                     &channel->channel_context,
                     device,
                     i,
                     kind,
                     (union moberg_channel_action) {
                       .digital_in.context=channel,
                       .digital_in.read=digital_in_read });
        map->map(map->device, &channel->channel);
      } break;
      case chan_DIGITALOUT: {
        struct moberg_channel_digital_out *channel = malloc(sizeof(*channel));

        if (! channel) { goto err_enomem; }        
        init_channel(&channel->channel,
                     channel,
                     &channel->channel_context,
                     device,
                     i,
                     kind,
                     (union moberg_channel_action) {
                       .digital_out.context=channel,
                       .digital_out.write=digital_out_write });
        map->map(map->device, &channel->channel);
      } break;
      case chan_ENCODERIN: {
        struct moberg_channel_encoder_in *channel = malloc(sizeof(*channel));
        
        if (! channel) { goto err_enomem; }
        init_channel(&channel->channel,
                     channel,
                     &channel->channel_context,
                     device,
                     i,
                     kind,
                     (union moberg_channel_action) {
                       .encoder_in.context=channel,
                       .encoder_in.read=encoder_in_read });
        map->map(map->device, &channel->channel);
      } break;
    }
  }
  return MOBERG_OK;
err_enomem:
  return MOBERG_ERRNO(ENOMEM);
syntax_err:
  return moberg_parser_failed(c, stderr);
}

static struct moberg_status start(struct moberg_device_context *device,
                                  FILE *f)
{
  fprintf(f, "# %s %s\n", __FILE__, __FUNCTION__);
  return MOBERG_OK;
}

static struct moberg_status stop(struct moberg_device_context *device,
                                 FILE *f)
{
  fprintf(f, "# %s %s\n", __FILE__, __FUNCTION__);
  return MOBERG_OK;
}

struct moberg_device_driver moberg_device_driver = {
  .new=new_context,
  .up=device_up,
  .down=device_down,
  .parse_config=parse_config,
  .parse_map=parse_map,
  .start=start,
  .stop=stop
};
