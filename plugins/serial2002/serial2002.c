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
#include <time.h>
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
    long timeout;
    struct serial2002_io io;
  } port;
  struct remap_analog {
    int count;
    struct analog_map {
      unsigned char index;
      unsigned long maxdata;
      double min;
      double max;
      double delta;
    } map[31];
  } analog_in, analog_out;
  struct remap_digital {
    int count;
    struct digital_map {
      unsigned char index;
    } map[32];
  } digital_in, digital_out, encoder_in;
  struct batch {
    int active;
    struct timespec expires;
    struct serial2002_data digital[32];
    struct serial2002_data channel[32];
  } batch;
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

static struct timespec timespec_sub(struct timespec t1, struct timespec t2)
{
  struct timespec result;
  result.tv_sec = t1.tv_sec - t2.tv_sec;
  result.tv_nsec = t1.tv_nsec - t2.tv_nsec;
  if (result.tv_nsec < 0) {
    result.tv_sec--;
    result.tv_nsec += 1000000000L;
  }
  return result;
}

static struct timespec timespec_add(struct timespec t1, struct timespec t2)
{
  struct timespec result;
  result.tv_sec = t1.tv_sec + t2.tv_sec;
  result.tv_nsec = t1.tv_nsec + t2.tv_nsec;
  if (result.tv_nsec > 1000000000L) {
    result.tv_sec++;
    result.tv_nsec -= 1000000000L;
  }
  return result;
}

static struct moberg_status batch_sampling(
  struct moberg_device_context *device,
  struct analog_map *analog,
  struct digital_map *digital,
  struct digital_map *encoder,
  struct serial2002_data *data)
{
  struct moberg_status result = MOBERG_OK;
  if (! data) {
    result = MOBERG_ERRNO(EINVAL);
    goto return_result;
  }
  struct timespec start, diff;
  if (clock_gettime(CLOCK_REALTIME, &start) < 0) {
    result = MOBERG_ERRNO(errno);
    goto return_result;
  }
  diff = timespec_sub(device->batch.expires, start);
  if (diff.tv_sec < 0 ||
      (digital && device->batch.digital[digital->index].kind != is_digital) ||
      (analog && device->batch.channel[analog->index].kind != is_channel) ||
      (encoder && device->batch.channel[encoder->index].kind != is_channel)) {
    /* Batch has expired */
    int expected = 0;
    for (int i = 0 ; i < device->digital_in.count ; i++) {
      struct digital_map *map = &device->digital_in.map[i];
      device->batch.digital[map->index].kind = is_invalid;
      expected++;
      result = serial2002_poll_digital(&device->port.io, map->index, 0);
      if (! OK(result)) {
        goto return_result;
      }
    }
    for (int i = 0 ; i < device->analog_in.count ; i++) {
      struct analog_map *map = &device->analog_in.map[i];
      device->batch.channel[map->index].kind = is_invalid;
      expected++;
      result = serial2002_poll_channel(&device->port.io, map->index, 0);
      if (! OK(result)) {
        goto return_result;
      }
    }
    for (int i = 0 ; i < device->encoder_in.count ; i++) {
      struct digital_map *map = &device->encoder_in.map[i];
      device->batch.channel[map->index].kind = is_invalid;
      expected++;
      result = serial2002_poll_channel(&device->port.io, map->index, 0);
      if (! OK(result)) {
        goto return_result;
      }
    }
    serial2002_flush(&device->port.io);
    while (expected > 0) {
      struct serial2002_data data;
      result = serial2002_read(&device->port.io, device->port.timeout, &data);
      if (! OK(result)) {
        goto return_result;
      }
      if (data.kind == is_digital) {
        device->batch.digital[data.index] = data;
        expected--;
      } else if (data.kind == is_channel) {
        device->batch.channel[data.index] = data;
        expected--;
      } else {
        result = MOBERG_ERRNO(EINVAL);
        goto return_result;
      }
    }
    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) < 0) {
      result = MOBERG_ERRNO(errno);
      goto return_result;
    }
    diff = timespec_sub(now, start);
    device->batch.expires = timespec_add(now, diff);
  }
  if (digital &&
      device->batch.digital[digital->index].kind == is_digital) {
    *data = device->batch.digital[digital->index];
    device->batch.digital[digital->index].kind = is_invalid;
  } else if (analog &&
             device->batch.channel[analog->index].kind == is_channel) {
    *data = device->batch.channel[analog->index];
    device->batch.channel[analog->index].kind = is_invalid;
  } else if (encoder &&
             device->batch.channel[encoder->index].kind == is_channel) {
    *data = device->batch.channel[encoder->index];
    device->batch.channel[encoder->index].kind = is_invalid;
  }

return_result:
  return result;
}

static struct moberg_status analog_in_read(
  struct moberg_channel_analog_in *analog_in,
  double *value)
{
  if (! value) { goto err_einval; }
  
  struct moberg_channel_context *channel = &analog_in->channel_context;
  struct moberg_device_context *device = channel->device;
  struct serial2002_data data;
  struct analog_map map = device->analog_in.map[channel->index];
  struct moberg_status result;

  if (device->batch.active) {
    result = batch_sampling(device, &map,  NULL, NULL, &data);
    if (! OK(result)) { goto return_result; }
  } else {
    result = serial2002_poll_channel(&device->port.io, map.index, 1);
    if (! OK(result)) { goto return_result; }
    result = serial2002_read(&device->port.io, device->port.timeout, &data);
    if (! OK(result)) { goto return_result; }
    if ((data.kind != is_channel) || (data.index != map.index)) {
      result = MOBERG_ERRNO(ECHRNG);
      goto return_result;
    }
  }
  *value = (data.value * map.delta + map.min);
return_result:
  return result;
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
  struct analog_map map = device->analog_out.map[channel->index];
  long as_long;
  if (desired_value < map.min) {
    as_long = 0;
  } else if (desired_value > map.max) {
    as_long = map.maxdata;
  } else {
    as_long = (desired_value - map.min) / map.delta;
  }
  if (as_long < 0) {
    as_long = 0;
  } else if (as_long > map.maxdata) {
    as_long = map.maxdata;
  }

  struct serial2002_data data = { is_channel, map.index, as_long };
  struct moberg_status result = serial2002_write(&device->port.io,  data, 1);
  if (OK(result) && actual_value) {
    *actual_value = data.value * map.delta + map.min;    
  }
  return result;
}

static struct moberg_status digital_in_read(
  struct moberg_channel_digital_in *digital_in,
  int *value)
{
  if (! value) { goto err_einval; }

  struct moberg_channel_context *channel = &digital_in->channel_context;
  struct moberg_device_context *device = channel->device;
  struct serial2002_data data = { 0, 0 };
  struct digital_map map = device->digital_in.map[channel->index];
  struct moberg_status result;

  if (device->batch.active) {
    result = batch_sampling(device, NULL, &map, NULL, &data);
    if (! OK(result)) { goto return_result; }
  } else {
    result = serial2002_poll_digital(&device->port.io, map.index, 1);
    if (! OK(result)) { goto return_result; }
    result = serial2002_read(&device->port.io, device->port.timeout, &data);
    if (! OK(result)) { goto return_result; }
    if ((data.kind != is_digital) || (data.index != map.index)) {
      result = MOBERG_ERRNO(ECHRNG);
      goto return_result;
    }
  }
  *value = data.value != 0;
return_result:
  return result;
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
  struct digital_map map = device->digital_out.map[channel->index];
  struct serial2002_data data = { is_digital, map.index, desired_value != 0 };
  struct moberg_status result = serial2002_write(&device->port.io,  data, 0);
  if (OK(result) && actual_value) {
    *actual_value = data.value;
  }
  return result;
}

static struct moberg_status encoder_in_read(
  struct moberg_channel_encoder_in *encoder_in,
  long *value)
{
  if (! value) { goto err_einval; }
  
  struct moberg_channel_context *channel = &encoder_in->channel_context;
  struct moberg_device_context *device = channel->device;
  struct serial2002_data data;
  struct digital_map map = device->encoder_in.map[channel->index];
  struct moberg_status result;
  if (device->batch.active) {
    result = batch_sampling(device, NULL, NULL, &map, &data);
    if (! OK(result)) { goto return_result; }
  } else {
    result = serial2002_poll_channel(&device->port.io, map.index, 1);
    if (! OK(result)) { goto return_result; }
    result = serial2002_read(&device->port.io, device->port.timeout, &data);
    if (! OK(result)) { goto return_result; }
    if ((data.kind != is_channel) || (data.index != map.index)) {
      result = MOBERG_ERRNO(ECHRNG);
      goto return_result;
    }
  }
  *value = (data.value);
return_result:
  return result;
err_einval:
  return MOBERG_ERRNO(EINVAL);
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
    result->port.timeout = 100000;
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
      remap->map[remap->count].maxdata = (1ULL << channel[i].bits) - 1;
      remap->map[remap->count].min = channel[i].min;
      remap->map[remap->count].max = channel[i].max;
      if (remap->map[remap->count].maxdata) {
        remap->map[remap->count].delta =
          (remap->map[remap->count].max - remap->map[remap->count].min) /
          remap->map[remap->count].maxdata;
      } else {
        remap->map[remap->count].delta = 1.0;

      }
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
      remap->count++;
    }
  }
}

static struct moberg_status device_open(struct moberg_device_context *device)
{
  struct moberg_status result;
  int fd = -1;

  if (device->port.count == 0) {
    fd = open(device->port.name, O_RDWR);
    if (fd < 0) { goto err_errno; }
    if (lockf(fd, F_TLOCK, 0)) { goto err_errno; }
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
      settings.flags |= ASYNC_LOW_LATENCY;
      /* It's expected for this to fail for at least some USB serial adapters */
      ioctl(fd, TIOCSSERIAL, &settings);
    }
    device->port.io.fd = fd;
    device->port.io.read.pos = 0;
    device->port.io.write.pos = 0;
    struct serial2002_config config;
    result = serial2002_read_config(&device->port.io,
                                    device->port.timeout, &config);
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
  }
  device->port.count++;
  return MOBERG_OK;
err_errno:
  result = MOBERG_ERRNO(errno);
err_result:
  if (fd >= 0) {
    lockf(fd, F_ULOCK, 0);
    close(fd);
  }
  return result;
}

static struct moberg_status device_close(struct moberg_device_context *device)
{
  if (device->port.count < 0) { errno = ENODEV; goto err_errno; }
  device->port.count--;
  if (device->port.count == 0) {
    lockf(device->port.io.fd, F_ULOCK, 0);
    if (close(device->port.io.fd) < 0) { goto err_errno; }
  }
  return MOBERG_OK;
err_errno:
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
  struct moberg_status result = device_open(channel->context->device);
  if (! OK(result)) {
    goto return_result;
  }
  int count = 0;
  switch (channel->kind) {
    case chan_ANALOGIN:
      count = channel->context->device->analog_in.count;
      break;
    case chan_ANALOGOUT:
      count = channel->context->device->analog_out.count;
      break;
    case chan_DIGITALIN:
      count = channel->context->device->digital_in.count;
      break;
    case chan_DIGITALOUT:
      count = channel->context->device->digital_out.count;
      break;
    case chan_ENCODERIN:
      count = channel->context->device->encoder_in.count;
      break;
  }
  if (channel->context->index >= count) {
     device_close(channel->context->device);
     result = MOBERG_ERRNO(ENODEV);
  }
return_result:
  return result;
}

static struct moberg_status channel_close(struct moberg_channel *channel)
{
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
    } else if (acceptkeyword(c, "timeout")) {
      token_t timeout;
      int multiplier = 0;
      if (! acceptsym(c, tok_EQUAL, NULL)) { goto syntax_err; }
      if (! acceptsym(c, tok_INTEGER, &timeout)) { goto syntax_err; }
      if (acceptkeyword(c, "s")) {
        multiplier = 1000000;
      } else if (acceptkeyword(c, "ms")) {
        multiplier = 1000;
      } else if (acceptkeyword(c, "us")) {
        multiplier = 1;
      } 
      if (! acceptsym(c, tok_SEMICOLON, NULL)) { goto syntax_err; }
      device->port.timeout = timeout.u.integer.value * multiplier;
    } else if (acceptkeyword(c, "batch_sampling")) {
      device->batch.active = 1;
      if (! acceptsym(c, tok_SEMICOLON, NULL)) { goto syntax_err; }
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
