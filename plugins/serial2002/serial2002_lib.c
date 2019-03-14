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
#include <moberg_inline.h>
#include <serial2002_lib.h>

static struct moberg_status tty_write(int fd, unsigned char *buf, int count)
{
  int n = 0;
  while (n < count) {
    int written = write(fd, &buf[n], count - n);
    if (written == 0) {
      fprintf(stderr, "Failed to write\n");
      return MOBERG_ERRNO(ENODATA);
    } else if (written < 0) {
      return MOBERG_ERRNO(errno);
    }
    n += written;
  }
  return MOBERG_OK;
}

static struct moberg_status tty_read(int fd, int timeout, unsigned char *value)
{
  struct pollfd pollfd;

  while (1) {
    pollfd.fd = fd;
    pollfd.events = POLLRDNORM | POLLRDBAND | POLLIN | POLLHUP | POLLERR;
    int err = poll(&pollfd, 1, timeout);
    if (err >= 1) {
      break;
    } else if (err == 0) {
      return MOBERG_ERRNO(ETIMEDOUT);
    } else if (err < 0) {
      return MOBERG_ERRNO(errno);
    }
  }
  int err = read(fd, value, 1);
  if (err == 1) {
    return MOBERG_OK;
  } else {
    return MOBERG_ERRNO(errno);
  }
}

struct moberg_status serial2002_poll_digital(int fd, int channel)
{
  unsigned char cmd;
  
  cmd = 0x40 | (channel & 0x1f);
  return tty_write(fd, &cmd, 1);
}

struct moberg_status serial2002_poll_channel(int fd, int channel)
{
  unsigned char cmd;
  
  cmd = 0x60 | (channel & 0x1f);
  return tty_write(fd, &cmd, 1);
}

struct moberg_status serial2002_read(int f, int timeout,
                                            struct serial2002_data *value)
{
  int length;
  
  value->kind = is_invalid;
  value->index = 0;
  value->value = 0;
  length = 0;
  while (1) {
    unsigned char data;
    struct moberg_status result = tty_read(f, timeout, &data);
    if (! OK(result)) {
      return result;
    }
    
    length++;
    if (data < 0) {
      fprintf(stderr, "serial2002 error\n");
      break;
    } else if (length < 6 && data & 0x80) {
      value->value = (value->value << 7) | (data & 0x7f);
    } else if (length == 6 && data & 0x80) {
      fprintf(stderr, "Too long\n");
      value->kind = is_invalid;
      break;
    } else {
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
        }
      } else {
        value->value = (value->value << 2) | ((data & 0x60) >> 5);
        value->kind = is_channel;
      }
      value->index = data & 0x1f;
      break;
    }
  }
  return MOBERG_OK;
}

struct moberg_status serial2002_write(int f, struct serial2002_data data)
{
  if (data.kind == is_digital) {
    unsigned char ch = ((data.value << 5) & 0x20) | (data.index & 0x1f);
    return tty_write(f, &ch, 1);
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
    return tty_write(f, ch, i);
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
  fprintf(stderr, "%f\n", result);
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

static struct moberg_status do_read_config(
  int fd,
  struct serial2002_config *config)
{
  struct serial2002_data data = { 0, 0 };

  memset(config, 0, sizeof(*config));
  serial2002_poll_channel(fd, 31);
  while (1) {
    struct moberg_status result = serial2002_read(fd, 1000, &data);
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
  int fd,
  struct serial2002_config *config)
{ 
  struct moberg_status result = do_read_config(fd, config);
  if (OK(result)) {
    result = check_config(config);
  }
  return result;
}


#if 0

static void x() {
  typedef struct {
    short int kind;
    short int bits;
    int min;
    int max;
  } config_t;
  config_t *dig_in_config;
  config_t *dig_out_config;
  config_t *chan_in_config;
  config_t *chan_out_config;
  int i;
  
  result = 0;
  dig_in_config = kcalloc(32, sizeof(config_t), GFP_KERNEL);
  dig_out_config = kcalloc(32, sizeof(config_t), GFP_KERNEL);
  chan_in_config = kcalloc(32, sizeof(config_t), GFP_KERNEL);
  chan_out_config = kcalloc(32, sizeof(config_t), GFP_KERNEL);
  if (!dig_in_config || !dig_out_config
      || !chan_in_config || !chan_out_config) {
    result = -ENOMEM;
    goto err_alloc_configs;
  }

  tty_setspeed(devpriv->tty, devpriv->speed);
  poll_channel(devpriv->tty, 31);	// Start reading configuration
  while (1) {
    struct serial2002_data data;
    
    data = serial_read(devpriv->tty, 1000);
    if (data.kind != is_channel || data.index != 31
        || !(data.value & 0xe0)) {
      break;
    } else {
      int command, channel, kind;
      config_t *cur_config = 0;
      
      channel = data.value & 0x1f;
      kind = (data.value >> 5) & 0x7;
      command = (data.value >> 8) & 0x3;
      switch (kind) {
        case 1:{
          cur_config = dig_in_config;
        }
          break;
        case 2:{
          cur_config = dig_out_config;
        }
          break;
        case 3:{
          cur_config = chan_in_config;
        }
          break;
        case 4:{
          cur_config = chan_out_config;
        }
          break;
        case 5:{
          cur_config = chan_in_config;
        }
          break;
      }
      
      if (cur_config) {
        cur_config[channel].kind = kind;
        switch (command) {
          case 0:{
            cur_config[channel].
              bits =
              (data.
               value >> 10) &
              0x3f;
          }
            break;
          case 1:{
            int unit, sign, min;
            unit = (data.
                    value >> 10) &
              0x7;
            sign = (data.
                    value >> 13) &
              0x1;
            min = (data.
                   value >> 14) &
              0xfffff;
            
            switch (unit) {
              case 0:{
                min = min * 1000000;
              }
                break;
              case 1:{
                min = min * 1000;
              }
                break;
              case 2:{
                min = min * 1;
              }
                break;
            }
            if (sign) {
              min = -min;
            }
            cur_config[channel].
              min = min;
          }
            break;
          case 2:{
            int unit, sign, max;
            unit = (data.
                    value >> 10) &
              0x7;
            sign = (data.
                    value >> 13) &
              0x1;
            max = (data.
                   value >> 14) &
              0xfffff;
            
            switch (unit) {
              case 0:{
                max = max * 1000000;
              }
                break;
              case 1:{
                max = max * 1000;
              }
                break;
              case 2:{
                max = max * 1;
              }
                break;
            }
            if (sign) {
              max = -max;
            }
            cur_config[channel].
              max = max;
          }
            break;
        }
      }
    }
  }
  for (i = 0; i <= 4; i++) {
    // Fill in subdev data
    config_t *c;
    unsigned char *mapping = 0;
    serial2002_range_table_t *range = 0;
    int kind = 0;
    
    switch (i) {
      case 0:{
        c = dig_in_config;
        mapping = devpriv->digital_in_mapping;
        kind = 1;
      }
        break;
      case 1:{
        c = dig_out_config;
        mapping = devpriv->digital_out_mapping;
        kind = 2;
      }
        break;
      case 2:{
        c = chan_in_config;
        mapping = devpriv->analog_in_mapping;
        range = devpriv->in_range;
        kind = 3;
      }
        break;
      case 3:{
        c = chan_out_config;
        mapping = devpriv->analog_out_mapping;
        range = devpriv->out_range;
        kind = 4;
      }
        break;
      case 4:{
        c = chan_in_config;
        mapping = devpriv->encoder_in_mapping;
        range = devpriv->in_range;
        kind = 5;
      }
        break;
      default:{
        c = 0;
      }
        break;
    }
    if (c) {
      comedi_subdevice *s;
      const comedi_lrange **range_table_list = NULL;
      unsigned int *maxdata_list;
      int j, chan;
      
      for (chan = 0, j = 0; j < 32; j++) {
        if (c[j].kind == kind) {
          chan++;
        }
      }
      s = &dev->subdevices[i];
      s->n_chan = chan;
      s->maxdata = 0;
      kfree(s->maxdata_list);
      s->maxdata_list = maxdata_list =
        kmalloc(sizeof(lsampl_t) * s->n_chan,
                GFP_KERNEL);
      if (!s->maxdata_list) {
        break;	/* error handled below */
      }
      kfree(s->range_table_list);
      s->range_table = 0;
      s->range_table_list = 0;
      if (kind == 1 || kind == 2) {
        s->range_table = &range_digital;
      } else if (range) {
        s->range_table_list = range_table_list =
          kmalloc(sizeof
                  (serial2002_range_table_t) *
                  s->n_chan, GFP_KERNEL);
        if (!s->range_table_list) {
          break;	/* error handled below */
        }
      }
      for (chan = 0, j = 0; j < 32; j++) {
        if (c[j].kind == kind) {
          if (mapping) {
            mapping[chan] = j;
          }
          if (range) {
            range[j].length = 1;
            range[j].range.min =
              c[j].min;
            range[j].range.max =
              c[j].max;
            range_table_list[chan] =
              (const
               comedi_lrange *)
              &range[j];
          }
          maxdata_list[chan] =
            ((long long)1 << c[j].
             bits) - 1;
          chan++;
        }
      }
    }
  }
  if (i <= 4) {
    /* Failed to allocate maxdata_list or range_table_list
     * for a subdevice that needed it. */
    result = -ENOMEM;
    for (i = 0; i <= 4; i++) {
      comedi_subdevice *s = &dev->subdevices[i];
      
      kfree(s->maxdata_list);
      s->maxdata_list = NULL;
      kfree(s->range_table_list);
      s->range_table_list = NULL;
    }
  }
err_alloc_configs:
  kfree(dig_in_config);
  kfree(dig_out_config);
  kfree(chan_in_config);
  kfree(chan_out_config);
  if (result) {
    if (devpriv->tty) {
      filp_close(devpriv->tty, 0);
      devpriv->tty = NULL;
    }
  }
}
return result;
}

static int serial2002_di_rinsn(comedi_device * dev, comedi_subdevice * s,
	comedi_insn * insn, lsampl_t * data)
{
	int n;
	int chan;

	chan = devpriv->digital_in_mapping[CR_CHAN(insn->chanspec)];
	for (n = 0; n < insn->n; n++) {
		struct serial2002_data read;

		poll_digital(devpriv->tty, chan);
		while (1) {
			read = serial_read(devpriv->tty, 1000);
			if (read.kind != is_digital || read.index == chan) {
				break;
			}
		}
		data[n] = read.value;
	}
	return n;
}

static int serial2002_do_winsn(comedi_device * dev, comedi_subdevice * s,
	comedi_insn * insn, lsampl_t * data)
{
	int n;
	int chan;

	chan = devpriv->digital_out_mapping[CR_CHAN(insn->chanspec)];
	for (n = 0; n < insn->n; n++) {
		struct serial2002_data write;

		write.kind = is_digital;
		write.index = chan;
		write.value = data[n];
		serial_write(devpriv->tty, write);
	}
	return n;
}

static int serial2002_ai_rinsn(comedi_device * dev, comedi_subdevice * s,
	comedi_insn * insn, lsampl_t * data)
{
	int n;
	int chan;

	chan = devpriv->analog_in_mapping[CR_CHAN(insn->chanspec)];
	for (n = 0; n < insn->n; n++) {
		struct serial2002_data read;

		poll_channel(devpriv->tty, chan);
		while (1) {
			read = serial_read(devpriv->tty, 1000);
			if (read.kind != is_channel || read.index == chan) {
				break;
			}
		}
		data[n] = read.value;
	}
	return n;
}

static int serial2002_ao_winsn(comedi_device * dev, comedi_subdevice * s,
	comedi_insn * insn, lsampl_t * data)
{
	int n;
	int chan;

	chan = devpriv->analog_out_mapping[CR_CHAN(insn->chanspec)];
	for (n = 0; n < insn->n; n++) {
		struct serial2002_data write;

		write.kind = is_channel;
		write.index = chan;
		write.value = data[n];
		serial_write(devpriv->tty, write);
		devpriv->ao_readback[chan] = data[n];
	}
	return n;
}

static int serial2002_ao_rinsn(comedi_device * dev, comedi_subdevice * s,
	comedi_insn * insn, lsampl_t * data)
{
	int n;
	int chan = CR_CHAN(insn->chanspec);

	for (n = 0; n < insn->n; n++) {
		data[n] = devpriv->ao_readback[chan];
	}

	return n;
}

static int serial2002_ei_rinsn(comedi_device * dev, comedi_subdevice * s,
	comedi_insn * insn, lsampl_t * data)
{
	int n;
	int chan;

	chan = devpriv->encoder_in_mapping[CR_CHAN(insn->chanspec)];
	for (n = 0; n < insn->n; n++) {
		struct serial2002_data read;

		poll_channel(devpriv->tty, chan);
		while (1) {
			read = serial_read(devpriv->tty, 1000);
			if (read.kind != is_channel || read.index == chan) {
				break;
			}
		}
		data[n] = read.value;
	}
	return n;
}

#endif
