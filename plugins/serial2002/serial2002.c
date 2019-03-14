/*
    serial2002.c -- serial2002 plugin for moberg

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
#include <serial2002_lib.h>

struct moberg_device_context {
  struct moberg *moberg;
  int (*dlclose)(void *dlhandle);
  void *dlhandle;
  int use_count;
  struct {
    int count;
    char *name;
    int baud;
    int fd;
  } port;
  struct remap_analog {
    int count;
    struct {
      unsigned char index;
      unsigned long maxdata;
      double min;
      double max;
      double delta;
    } map[31];
  } analog_in, analog_out;
  struct remap_digital {
    int count;
    struct {
      unsigned char index;
    } map[32];
  } digital_in, digital_out,
    encoder_in;
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
  struct serial2002_data data = { 0, 0 };
  serial2002_poll_channel(device->port.fd,
                          device->analog_in.map[channel->index].index);
  struct moberg_status result = serial2002_read(device->port.fd, 1000, &data);
  if (OK(result)) {
    *value = (data.value * device->analog_in.map[channel->index].delta +
              device->analog_in.map[channel->index].min);
  }
  return result;
err_einval:
  return MOBERG_ERRNO(EINVAL);
}

static struct moberg_status analog_out_write(
  struct moberg_channel_analog_out *analog_out,
  double value)
{
  fprintf(stderr, "%s\n", __FUNCTION__);
  return MOBERG_OK;
}

static struct moberg_status digital_in_read(
  struct moberg_channel_digital_in *digital_in,
  int *value)
{
  fprintf(stderr, "%s\n", __FUNCTION__);
  *value = 1;
  return MOBERG_OK;
}

static struct moberg_status digital_out_write(
  struct moberg_channel_digital_out *digital_out,
  int value)
{
  fprintf(stderr, "%s\n", __FUNCTION__);
  return MOBERG_OK;
}

static struct moberg_status encoder_in_read(struct moberg_channel_encoder_in *encoder_in,
                           long *value)
{
  fprintf(stderr, "%s\n", __FUNCTION__);
  return MOBERG_OK;
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
    free(context->port.name);
    free(context);
    return 0;
  }
  return context->use_count;
}

static void remap_analog(
  struct remap_analog *remap,
  enum serial2002_kind kind,
  struct serial2002_channel *channel,
  int count)
{
  remap->count = 0;
  for (int i = 0 ; i < count ; i++) {
    if (channel[i].kind == kind) {
      remap->map[remap->count].index = i;
      remap->map[remap->count].maxdata = (1 << channel[i].bits) - 1;
      remap->map[remap->count].min = channel[i].min;
      remap->map[remap->count].max = channel[i].max;
      if (remap->map[remap->count].maxdata) {
        remap->map[remap->count].delta =
          (remap->map[remap->count].max - remap->map[remap->count].min) /
          remap->map[remap->count].maxdata;
      }

      fprintf(stderr, "%d -> %d\n", remap->count, i);
      remap->count++;
    }
  }
}

static void remap_digital(
  struct remap_digital *remap,
  enum serial2002_kind kind,
  struct serial2002_channel *channel,
  int count)
{
  remap->count = 0;
  for (int i = 0 ; i < count ; i++) {
    if (channel[i].kind == kind) {
      remap->map[remap->count].index = i;
      fprintf(stderr, "%d -> %d\n", remap->count, i);
      remap->count++;
    }
  }
}

static struct moberg_status device_open(struct moberg_device_context *device)
{
  struct moberg_status result;
  int fd = -1;

  fprintf(stderr, "%s\n", __FUNCTION__);
  if (device->port.count == 0) {
    fd = open(device->port.name, O_RDWR);
    if (fd < 0) { goto err_errno; }
    struct termios2 termios2;
    if (ioctl(fd, TCGETS2, &termios2) < 0) { goto err_errno; }
    termios2.c_iflag = 0;
    termios2.c_oflag = 0;
    termios2.c_lflag = 0;
    termios2.c_cflag = CLOCAL | CS8 | CREAD | BOTHER;
    termios2.c_cc[VMIN] = 0;
    termios2.c_cc[VTIME] = 0;
    termios2.c_ispeed = device->port.baud;
    termios2.c_ospeed = device->port.baud;
    if (ioctl(fd, TCSETS2, &termios2) < 0) { goto err_errno; }
    struct serial_struct settings; 
    if (ioctl(fd, TIOCGSERIAL, &settings) >= 0) {
      /* It's expected for this to fail for at least some USB serial adapters */
      ioctl(fd, TIOCSSERIAL, &settings);
    }
    struct serial2002_config config;
    result = serial2002_read_config(fd, &config);
    if (! OK(result)) { goto err_result; }
    remap_analog(&device->analog_in, SERIAL2002_ANALOG_IN,
                  config.channel_in, 31);
    remap_analog(&device->analog_out, SERIAL2002_ANALOG_OUT,
                  config.channel_out, 31);
    remap_digital(&device->digital_in, SERIAL2002_DIGITAL_IN,
                  config.digital_in, 32);
    remap_digital(&device->digital_out, SERIAL2002_DIGITAL_OUT,
                  config.digital_out, 32);
    remap_digital(&device->encoder_in, SERIAL2002_COUNTER_IN,
                  config.channel_in, 31);
    device->port.fd = fd;
  }
  device->port.count++;
  fprintf(stderr, "OPENED %d\n", device->port.count);
  return MOBERG_OK;
err_errno:
  fprintf(stderr, "ERRNO %d\n", errno);
  result = MOBERG_ERRNO(errno);
err_result:
  if (fd >= 0) {
    close(fd);
  }
  return result;
}

static struct moberg_status device_close(struct moberg_device_context *device)
{
  fprintf(stderr, "%s\n", __FUNCTION__);
  if (device->port.count < 0) { errno = ENODEV; goto err_errno; }
  device->port.count--;
  if (device->port.count == 0) {
    fprintf(stderr, "CLOSE\n");
    if (close(device->port.fd) < 0) { goto err_errno; }
  }
  return MOBERG_OK;
err_errno:
  fprintf(stderr, "CLOSE %d\n", errno);
  return MOBERG_ERRNO(errno);

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
  fprintf(stderr, "%s\n", __FUNCTION__);
  struct moberg_status result = device_open(channel->context->device);
  return result;
}

static struct moberg_status channel_close(struct moberg_channel *channel)
{
  fprintf(stderr, "%s\n", __FUNCTION__);
  struct moberg_status result = device_close(channel->context->device);
  return result;
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
    } else if (acceptkeyword(c, "device")) {
      token_t name;
      if (! acceptsym(c, tok_EQUAL, NULL)) { goto syntax_err; }
      if (! acceptsym(c, tok_STRING, &name)) { goto syntax_err; }
      if (! acceptsym(c, tok_SEMICOLON, NULL)) { goto syntax_err; }
      device->port.name = strndup(name.u.idstr.value, name.u.idstr.length);
      if (! device->port.name) { goto err_enomem; }
    } else if (acceptkeyword(c, "baud")) {
      token_t baud;
      if (! acceptsym(c, tok_EQUAL, NULL)) { goto syntax_err; }
      if (! acceptsym(c, tok_INTEGER, &baud)) { goto syntax_err; }
      if (! acceptsym(c, tok_SEMICOLON, NULL)) { goto syntax_err; }
      device->port.baud = baud.u.integer.value;
    } else {
      goto syntax_err;
    }
  }
  return MOBERG_OK;
err_enomem:
  return MOBERG_ERRNO(ENOMEM);  
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
  return MOBERG_OK;
}

static struct moberg_status stop(struct moberg_device_context *device,
                                 FILE *f)
{
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
