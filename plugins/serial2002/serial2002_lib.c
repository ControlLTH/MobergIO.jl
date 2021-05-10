/*
    serial2002_lib.c -- serial2002 protocol

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

#include <unistd.h>
#include <stdio.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <moberg_inline.h>
#include <serial2002_lib.h>

struct moberg_status serial2002_flush(struct serial2002_io *io)
{
  int n = 0;
  while (n < io->write.pos) {
    int written = write(io->fd, &io->write.data[n], io->write.pos - n);
    if (written == 0) {
      return MOBERG_ERRNO(ENODATA);
    } else if (written < 0) {
      return MOBERG_ERRNO(errno);
    }
    n += written;
  }
  io->write.pos = 0;
  return MOBERG_OK;
}

static struct moberg_status tty_write(struct serial2002_io *io,
                                      unsigned char *buf,
                                      int count,
                                      int flush)
{
  struct moberg_status result = MOBERG_OK;

  for (int i = 0 ; i < count ; i++) {
    io->write.data[io->write.pos] = buf[i];
    io->write.pos++;
    if (io->write.pos >= sizeof(io->write.data)) {
      result = serial2002_flush(io);
      if (! OK(result)) {
        goto return_result;
      }
    }
  }
  if (flush) {
    result = serial2002_flush(io);
    if (! OK(result)) {
      goto return_result;
    }
  }
return_result:
  return result;
}

static struct moberg_status tty_read(struct serial2002_io *io,
                                     long timeout,
                                     unsigned char *value)
{
  struct moberg_status result = MOBERG_OK;

  if (io->read.pos >= io->read.count) {
    struct pollfd pollfd;
    pollfd.fd = io->fd;
    pollfd.events = POLLRDNORM | POLLRDBAND | POLLIN | POLLHUP | POLLERR;
    int err = poll(&pollfd, 1, timeout / 1000);
    if (err == 0) {
      result = MOBERG_ERRNO(ETIMEDOUT);
      goto return_result;

    } else if (err < 0) {
      result = MOBERG_ERRNO(errno);
      goto return_result;
    }
    int available;
    err = ioctl(io->fd, FIONREAD, &available);
    if (err < 0) {
      result = MOBERG_ERRNO(errno);
      goto return_result;
    }
    if (available > sizeof(io->read.data)) {
      available = sizeof(io->read.data);
    }
    err = read(io->fd, &io->read.data[0], available);
    if (err > 0) {
      io->read.pos = 0;
      io->read.count = err;
    } else if (err == 0) {
      result = MOBERG_ERRNO(ENODATA);
      goto return_result;
    } else if (err < 0) {
      result = MOBERG_ERRNO(errno);
      goto return_result;
    }
  }
return_result:
  *value = io->read.data[io->read.pos];
  io->read.pos++;
  return result;
}

struct moberg_status serial2002_poll_digital(struct serial2002_io *io,
                                             int channel,
                                             int flush)
{
  unsigned char cmd;
  
  cmd = 0x40 | (channel & 0x1f);
  return tty_write(io, &cmd, 1, flush);
}

struct moberg_status serial2002_poll_channel(struct serial2002_io *io,
                                             int channel,
                                             int flush)
{
  unsigned char cmd;
  
  cmd = 0x60 | (channel & 0x1f);
  return tty_write(io, &cmd, 1, flush);
}

struct moberg_status serial2002_read(struct serial2002_io *io,
                                     long timeout,
                                     struct serial2002_data *value)
{
  int length;
  
  value->kind = is_invalid;
  value->index = 0;
  value->value = 0;
  length = 0;
  while (value->kind == is_invalid) {
    unsigned char data;
    struct moberg_status result = tty_read(io, timeout, &data);
    if (! OK(result)) {
      return result;
    }
    
    length++;
    if (length < 6 && data & 0x80) {
      value->value = (value->value << 7) | (data & 0x7f);
    } else if (length == 6 && data & 0x80) {
      value->kind = is_invalid;
      return MOBERG_ERRNO(EFBIG);
      break;
    } else {
      value->index = data & 0x1f;
      if (length == 1) {
        switch ((data >> 5) & 0x03) {
          case 0:{
            value->value = 0;
            value->kind = is_digital;
          } break;
          case 1:{
            value->value = 1;
            value->kind = is_digital;
          } break;
          case 2:{
            if (data == 0x5e) {
              /* Ignore FTDI USB/serial event character (get bit 30) */
              value->kind = is_invalid;
              value->index = 0;
              value->value = 0;
              length = 0;
            } else {
              return MOBERG_ERRNO(EINVAL);
            }
          } break;
          case 3:{
            return MOBERG_ERRNO(EINVAL);
          } break;
        }
      } else {
        value->value = (value->value << 2) | ((data & 0x60) >> 5);
        value->kind = is_channel;
      }
    }
  }
  return MOBERG_OK;
}

struct moberg_status serial2002_write(struct serial2002_io *io,
                                      struct serial2002_data data,
                                      int flush)
{
  if (data.kind == is_digital) {
    unsigned char ch = ((data.value << 5) & 0x20) | (data.index & 0x1f);
    return tty_write(io, &ch, 1, 1);
  } else {
    unsigned char ch[6];
    int i = 0;
    if (data.value >= (1L << 30)) {
      ch[i] = 0x80 | ((data.value >> 30) & 0x03);
      i++;
    }
    if (data.value >= (1L << 23)) {
      ch[i] = 0x80 | ((data.value >> 23) & 0x7f);
      i++;
    }
    if (data.value >= (1L << 16)) {
      ch[i] = 0x80 | ((data.value >> 16) & 0x7f);
      i++;
    }
    if (data.value >= (1L << 9)) {
      ch[i] = 0x80 | ((data.value >> 9) & 0x7f);
      i++;
    }
    ch[i] = 0x80 | ((data.value >> 2) & 0x7f);
    i++;
    ch[i] = ((data.value << 5) & 0x60) | (data.index & 0x1f);
    i++;
    return tty_write(io, ch, i, 1);
  }
}

static double interpret_value(int value)
{
  unsigned int unit = (value & 0x7);
  unsigned int sign = (value & 0x8) >> 3;
  double result = (value & ~0xf) >> 4;
  if (sign == 1) {
    result = -result;
  }
  switch (unit) {
    case 1:
      result *= 1e-3;
      break;
    case 2:
      result *= 1e-6;
      break;
  }
  return result;
}

static struct moberg_status update_channel(struct serial2002_channel *channel,
                                           unsigned int kind,
                                           unsigned int cmd,
                                           unsigned int value)
{
  if (channel->kind == 0) {
    channel->kind = kind;
  } else if (channel->kind != kind) {
    return MOBERG_ERRNO(EINVAL);
  }
  channel->valid |= 1 << cmd;
  switch (cmd) {
    case 0:
      channel->bits = value;
      break;
    case 1:
      channel->min = interpret_value(value);
      break;
    case 2:
      channel->max = interpret_value(value);
      break;
  }
  return MOBERG_OK;
}

static void discard_pending(struct serial2002_io *io)
{
  struct pollfd pollfd;

  while (1) {
    pollfd.fd = io->fd;
    pollfd.events = POLLRDNORM | POLLRDBAND | POLLIN | POLLHUP | POLLERR;
    int err = poll(&pollfd, 1, 0);
    if (err <= 0) {
      break;
    } else {
      char discard;
      err = read(io->fd, &discard, 1);
      if (err <= 0) {
        break;
      }
    }
  }
}

static struct moberg_status do_read_config(
  struct serial2002_io *io,
  long timeout,
  struct serial2002_config *config)
{
  struct serial2002_data data = { 0, 0 };

  discard_pending(io);
  memset(config, 0, sizeof(*config));
  serial2002_poll_channel(io, 31, 1);
  while (1) {
    struct moberg_status result = serial2002_read(io, timeout, &data);
    if (! OK(result)) { return result; }
    unsigned int channel = (data.value & 0x0001f);
    unsigned int kind = (data.value & 0x00e0) >> 5;
    unsigned int cmd = (data.value & 0x0300) >> 8;
    unsigned int value = (data.value & ~0x3ff) >> 10;
    switch (kind) {
      case 0: return MOBERG_OK;
      case 1: /* digital in */
        config->digital_in[channel].valid = 1;
        config->digital_in[channel].kind = kind;
        break;
      case 2: /* digital out */
        config->digital_out[channel].valid = 1;
        config->digital_out[channel].kind = kind;
        break;
      case 3: /* analog in */
        update_channel(&config->channel_in[channel], kind, cmd, value);
        break;
      case 4: /* analog out */
        update_channel(&config->channel_out[channel], kind, cmd, value);
        break;
      case 5: /* counter in */
        update_channel(&config->channel_in[channel], kind, cmd, value);
        break;
    }
  }
}

static struct moberg_status check_config(struct serial2002_config *c)
{
  struct moberg_status result = MOBERG_OK;
  
  for (int i = 0 ; i < 31 ; i++) {
    switch (c->channel_in[i].kind) {
      case 3: /* analog in */
        if (c->channel_in[i].valid && c->channel_in[i].valid != 0x7) {
          fprintf(stderr, "Analog input channel[%d] is missing ", i);
          if (! (c->channel_in[i].valid & 0x1)) { fprintf(stderr, "#bits "); }
          if (! (c->channel_in[i].valid & 0x2)) { fprintf(stderr, "min "); }
          if (! (c->channel_in[i].valid & 0x4)) { fprintf(stderr, "max "); }
          fprintf(stderr, "\n");
          result = MOBERG_ERRNO(EINVAL);
        }
        break;
      case 4: /* analog out */
        if (c->channel_out[i].valid && c->channel_out[i].valid != 0x7) {
          fprintf(stderr, "Analog output channel[%d] is missing ", i);
          if (! (c->channel_out[i].valid & 0x1)) { fprintf(stderr, "#bits "); }
          if (! (c->channel_out[i].valid & 0x2)) { fprintf(stderr, "min "); }
          if (! (c->channel_out[i].valid & 0x4)) { fprintf(stderr, "max "); }
          fprintf(stderr, "\n");
          result = MOBERG_ERRNO(EINVAL);
        }
      case 5: /* counter in */
        if (c->channel_in[i].valid && c->channel_in[i].valid != 0x1) {
          fprintf(stderr, "Counter input channel[%d] is missing #bits\n", i);
          result = MOBERG_ERRNO(EINVAL);
        }
        break;
    }
  }
  return result;
}

struct moberg_status serial2002_read_config(
  struct serial2002_io *io,
  long timeout,
  struct serial2002_config *config)
{ 
  struct moberg_status result = do_read_config(io, timeout, config);
  if (OK(result)) {
    result = check_config(config);
  }
  return result;
}


