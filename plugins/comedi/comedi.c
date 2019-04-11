/*
    comedi.c -- comedi plugin for moberg

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
    
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <comedilib.h>
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
  char *name;
  struct {
    int count;
    comedi_t *handle;
  } comedi;
  struct idstr {
    struct idstr *next;
    struct idstr *prev;
    kind_t kind;
    char *value;
  } modprobe_list, config_list;
};

struct moberg_channel_context {
  void *to_free; /* To be free'd when use_count goes to zero */
  struct moberg_device_context *device;
  int use_count;
  struct channel_descriptor {
    int subdevice;
    int subchannel;
    int route;
    lsampl_t maxdata;
    double min;
    double max;
    double delta;
  } descriptor;
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
  struct channel_descriptor descriptor = analog_in->channel_context.descriptor;
  lsampl_t data;
  if (0 > comedi_data_read(analog_in->channel_context.device->comedi.handle,
                           descriptor.subdevice,
                           descriptor.subchannel,
                           0, 0, &data)) {
    goto err_errno;
  }
  *value = descriptor.min + data * descriptor.delta;
  return MOBERG_OK;
err_einval:
  return MOBERG_ERRNO(EINVAL);
err_errno:
  return MOBERG_ERRNO(comedi_errno());
}

static struct moberg_status analog_out_write(
  struct moberg_channel_analog_out *analog_out,
  double desired_value,
  double *actual_value)
{
  struct channel_descriptor descriptor = analog_out->channel_context.descriptor;
  lsampl_t data;
  if (desired_value < descriptor.min) {
    data = 0;
  } else if (desired_value > descriptor.max) {
    data = descriptor.maxdata;
  } else {
    data = (desired_value - descriptor.min) / descriptor.delta;
  }
  if (data < 0) {
    data = 0;
  } else if (data > descriptor.maxdata) {
    data = descriptor.maxdata;
  }
  if (0 > comedi_data_write(analog_out->channel_context.device->comedi.handle,
                            descriptor.subdevice,
                            descriptor.subchannel,
                            0, 0, data)) {
    goto err_errno;
  }
  if (actual_value) {
    *actual_value = data * descriptor.delta + descriptor.min;
  }
    
  return MOBERG_OK;
err_errno:
  return MOBERG_ERRNO(comedi_errno());
}

static struct moberg_status digital_in_read(
  struct moberg_channel_digital_in *digital_in,
  int *value)
{
  if (! value) { goto err_einval; }
  struct channel_descriptor descriptor = digital_in->channel_context.descriptor;
  lsampl_t data;
  if (0 > comedi_data_read(digital_in->channel_context.device->comedi.handle,
                           descriptor.subdevice,
                           descriptor.subchannel,
                           0, 0, &data)) {
    goto err_errno;
  }
  *value = data;
  return MOBERG_OK;
err_einval:
  return MOBERG_ERRNO(EINVAL);
err_errno:
  return MOBERG_ERRNO(comedi_errno());
}

static struct moberg_status digital_out_write(
  struct moberg_channel_digital_out *digital_out,
  int desired_value,
  int *actual_value)
{
  struct channel_descriptor descriptor = digital_out->channel_context.descriptor;
  lsampl_t data = desired_value==0?0:1;
  if (0 > comedi_data_write(digital_out->channel_context.device->comedi.handle,
                            descriptor.subdevice,
                            descriptor.subchannel,
                            0, 0, data)) {
    goto err_errno;
  }
  if (actual_value) {
    *actual_value = data;
  }
  return MOBERG_OK;
err_errno:
  return MOBERG_ERRNO(comedi_errno());
}

static struct moberg_status encoder_in_read(struct moberg_channel_encoder_in *encoder_in,
                           long *value)
{
  if (! value) { goto err_einval; }
  struct channel_descriptor descriptor = encoder_in->channel_context.descriptor;
  lsampl_t data;
   if (0 > comedi_data_read(encoder_in->channel_context.device->comedi.handle,
                            descriptor.subdevice,
                            descriptor.subchannel,
                            0, 0, &data)) {
    goto err_errno;
  }
  *value = data - descriptor.maxdata / 2;
  return MOBERG_OK;
err_einval:
  return MOBERG_ERRNO(EINVAL);
err_errno:
  return MOBERG_ERRNO(comedi_errno());
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
    result->modprobe_list.next = &result->modprobe_list;
    result->modprobe_list.prev = &result->modprobe_list;
    result->config_list.next = &result->config_list;
    result->config_list.prev = &result->config_list;
  }
  return result;
}

static int device_up(struct moberg_device_context *device)
{
  device->use_count++;
  return device->use_count;
}

static int device_down(struct moberg_device_context *device)
{
  device->use_count--;
  int result = device->use_count;
  if (device->use_count <= 0) {
    moberg_deferred_action(device->moberg,
                           device->dlclose, device->dlhandle);
    free(device->name);
    struct idstr *e;
    
    e = device->modprobe_list.next;
    while (e != &device->modprobe_list) {
      struct idstr *next = e->next;
      free(e->value);
      free(e);
      e = next;
    }
    e = device->config_list.next;
    while (e != &device->config_list) {
      struct idstr *next = e->next;
      free(e->value);
      free(e);
      e = next;
    }
    free(device);
    return 0;
  }
  return result;
}

static struct moberg_status device_open(struct moberg_device_context *device)
{
  if (device->comedi.count == 0) {
    device->comedi.handle = comedi_open(device->name);
    if (device->comedi.handle == NULL) {
      goto err_errno;
    }
  }
  device->comedi.count++;
  return MOBERG_OK;
err_errno:
  fprintf(stderr, "Failed to open %s\n", device->name);
  return MOBERG_ERRNO(errno);
}

static struct moberg_status device_close(struct moberg_device_context *device)
{
  device->comedi.count--;
  if (device->comedi.count == 0) {
    if (comedi_close(device->comedi.handle)) {
      goto err_errno;
    }
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
  if (! OK(result)) { goto err_result; }
  channel_up(channel);
  lsampl_t maxdata;
  comedi_range *range;
          
  maxdata = comedi_get_maxdata(channel->context->device->comedi.handle,
                               channel->context->descriptor.subdevice,
                               channel->context->descriptor.subchannel);
  range = comedi_get_range(channel->context->device->comedi.handle,
                           channel->context->descriptor.subdevice,
                           channel->context->descriptor.subchannel,
                           0);

  if (! maxdata) {
    fprintf(stderr, "Failed to get maxdata for %s[%d][%d]\n",
            channel->context->device->name, 
            channel->context->descriptor.subdevice, 
            channel->context->descriptor.subchannel);
    goto err_enodata;
  }
  if (! range) {
    fprintf(stderr, "Failed to get range for %s[%d][%d]\n",
            channel->context->device->name, 
            channel->context->descriptor.subdevice, 
            channel->context->descriptor.subchannel);
    goto err_enodata;
  }
  channel->context->descriptor.maxdata = maxdata;
  channel->context->descriptor.min = range->min;
  channel->context->descriptor.max = range->max;
  channel->context->descriptor.delta = (range->max - range->min) / maxdata;
  if (channel->kind == chan_DIGITALIN) {
    if (0 > comedi_dio_config(channel->context->device->comedi.handle,
                              channel->context->descriptor.subdevice,
                              channel->context->descriptor.subchannel,
                              0) && errno != ENOENT) {
      goto err_errno;
    }
  } else if (channel->kind == chan_DIGITALOUT) {
    if (0 > comedi_dio_config(channel->context->device->comedi.handle,
                              channel->context->descriptor.subdevice,
                              channel->context->descriptor.subchannel,
                              1) && errno != ENOENT) {
      goto err_errno;
    }
  }
  if (channel->context->descriptor.route != -1) {
    comedi_insn insn;
    lsampl_t data[2];
    memset(&insn, 0, sizeof(comedi_insn));
    insn.insn = INSN_CONFIG;
    insn.subdev = channel->context->descriptor.subdevice;
    insn.chanspec = channel->context->descriptor.subchannel;
    insn.data = data;
    insn.n = sizeof(data) / sizeof(data[0]);
    data[0] = INSN_CONFIG_SET_ROUTING;
    data[1] = channel->context->descriptor.route;
    if (0 > comedi_do_insn(channel->context->device->comedi.handle, &insn)) {
      goto err_errno;
    }
  }
  return MOBERG_OK;
err_errno:
  return MOBERG_ERRNO(errno);
err_enodata:
  return MOBERG_ERRNO(ENODATA);
err_result:
  return result;
}

static struct moberg_status channel_close(struct moberg_channel *channel)
{
  channel_down(channel);
  struct moberg_status result = device_close(channel->context->device);
  if (! OK(result)) { goto err_result; }
  return MOBERG_OK;
err_result:
  return result;
}

static void init_channel(struct moberg_channel *channel,
                         void *to_free,
                         struct moberg_channel_context *context,
                         struct moberg_device_context *device,
                         struct channel_descriptor descriptor,
                         enum moberg_channel_kind kind,
                         union moberg_channel_action action)
{
  context->to_free = to_free;
  context->device = device;
  context->use_count = 0;
  context->descriptor = descriptor;
  
  channel->context = context;
  channel->up = channel_up;
  channel->down = channel_down;
  channel->open = channel_open;
  channel->close = channel_close;
  channel->kind = kind;
  channel->action = action;
};

static struct moberg_status append_modprobe(
  struct moberg_device_context *device,
  token_t token)
{
  struct idstr *modprobe = malloc(sizeof(*modprobe));
  if (! modprobe) { goto err_enomem; }
  modprobe->value = strndup(token.u.idstr.value, token.u.idstr.length);
  if (! modprobe->value) { goto free_modprobe; }
  modprobe->prev = device->modprobe_list.prev;
  modprobe->next = modprobe->prev->next;
  modprobe->prev->next = modprobe;
  modprobe->next->prev = modprobe;
  return MOBERG_OK;
free_modprobe:
  free(modprobe);
err_enomem:
  return MOBERG_ERRNO(ENOMEM);
}

static struct moberg_status append_config(
  struct moberg_device_context *device,
  token_t token)
{
  struct idstr *config = malloc(sizeof(*config));
  if (! config) { goto err_enomem; }
  config->value = strndup(token.u.idstr.value, token.u.idstr.length);
  if (! config->value) { goto free_config; }
  config->prev = device->config_list.prev;
  config->next = config->prev->next;
  config->prev->next = config;
  config->next->prev = config;
  return MOBERG_OK;
free_config:
  free(config);
err_enomem:
  return MOBERG_ERRNO(ENOMEM);
}
  
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
      device->name = strndup(name.u.idstr.value, name.u.idstr.length);
      if (! device->name) { goto err_enomem; }
    } else if (acceptkeyword(c, "config")) {
      if (! acceptsym(c, tok_EQUAL, NULL)) { goto syntax_err; }
      if (! acceptsym(c, tok_LBRACKET, NULL)) { goto syntax_err; }
      for (;;) {
        token_t config;
	if (acceptsym(c, tok_RBRACKET, NULL)) {
	  break;
	} else if (acceptsym(c, tok_IDENT, &config) ||
		   acceptsym(c, tok_STRING, &config)) {
          append_config(device, config);
	} else {
	  goto syntax_err;
	}
      }
      if (! acceptsym(c, tok_SEMICOLON, NULL)) { goto syntax_err; }
    } else if (acceptkeyword(c, "modprobe")) {
      if (! acceptsym(c, tok_EQUAL, NULL)) { goto syntax_err; }
      if (! acceptsym(c, tok_LBRACKET, NULL)) { goto syntax_err; }
      for (;;) {
        token_t modprobe;
	if (acceptsym(c, tok_RBRACKET, NULL)) {
	  break;
	} else if (acceptsym(c, tok_IDENT, &modprobe) ||
		   acceptsym(c, tok_STRING, &modprobe)) {
          append_modprobe(device, modprobe);
	} else {
	  goto syntax_err;
	}
      }
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
  enum moberg_channel_kind kind,
  struct moberg_channel_map *map)
{
  token_t min, max, route={ .u.integer.value=-1 };

  if (! acceptsym(c, tok_LBRACE, NULL)) { goto syntax_err; }
  for (;;) {
    token_t subdevice;
    if (! acceptkeyword(c, "subdevice") != 0) {  goto syntax_err; }
    if (! acceptsym(c, tok_LBRACKET, NULL)) { goto syntax_err; }
    if (! acceptsym(c, tok_INTEGER, &subdevice)) { goto syntax_err; }
    if (! acceptsym(c, tok_RBRACKET, NULL)) { goto syntax_err; }
    if (acceptkeyword(c, "route")) {
      if (! acceptsym(c, tok_INTEGER, &route)) { goto syntax_err; }
    }
    if (! acceptsym(c, tok_LBRACKET, NULL)) { goto syntax_err; }
    if (! acceptsym(c, tok_INTEGER, &min)) { goto syntax_err; }
    if (acceptsym(c, tok_COLON, NULL)) { 
      if (! acceptsym(c, tok_INTEGER, &max)) { goto syntax_err; }
    } else {
      max = min;
    }
    for (int i = min.u.integer.value ; i <= max.u.integer.value ; i++) {
      struct channel_descriptor descriptor = {
        .subdevice=subdevice.u.integer.value,
        .subchannel=i,
        .route=route.u.integer.value,
        .maxdata=0,
        .min=0.0,
        .max=0.0
      };
      switch (kind) {
        case chan_ANALOGIN: {
          struct moberg_channel_analog_in *channel = malloc(sizeof(*channel));
          
          if (! channel) { goto err_enomem; }
          init_channel(&channel->channel,
                       channel,
                       &channel->channel_context,
                       device,
                       descriptor,
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
                       descriptor,
                       kind,
                       (union moberg_channel_action) {
                         .analog_out.context=channel,
                         .analog_out.write=analog_out_write });
          map->map(map->device, &channel->channel);
        } break;
        case chan_DIGITALIN: {
          struct moberg_channel_digital_in *channel = malloc(sizeof(*channel));
          
          if (!channel)  { goto err_enomem; }
          init_channel(&channel->channel,
                       channel,
                       &channel->channel_context,
                       device,
                       descriptor,
                       kind,
                       (union moberg_channel_action) {
                         .digital_in.context=channel,
                         .digital_in.read=digital_in_read });
          map->map(map->device, &channel->channel);
        } break;
        case chan_DIGITALOUT: {
          struct moberg_channel_digital_out *channel = malloc(sizeof(*channel));
          
          if (!channel) { goto err_enomem; }
          init_channel(&channel->channel,
                       channel,
                       &channel->channel_context,
                       device,
                       descriptor,
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
                       descriptor,
                       kind,
                       (union moberg_channel_action) {
                         .encoder_in.context=channel,
                         .encoder_in.read=encoder_in_read });
          map->map(map->device, &channel->channel);
        } break;
      }
    }
    if (! acceptsym(c, tok_RBRACKET, NULL)) { goto syntax_err; }
    if (! acceptsym(c, tok_COMMA, NULL)) { break; }
    
  }
  if (! acceptsym(c, tok_RBRACE, NULL)) { goto syntax_err; }
  return MOBERG_OK;
err_enomem:
  return MOBERG_ERRNO(ENOMEM);
syntax_err:
  return moberg_parser_failed(c, stderr);
}

static struct moberg_status start(struct moberg_device_context *device,
                                  FILE *f)
{
  for (struct idstr *e = device->modprobe_list.next ;
       e != &device->modprobe_list ;
       e = e->next) {
    fprintf(f, "modprobe %s\n", e->value);
  }
  fprintf(f, "comedi_config %s ", device->name);
  int i = 0;
  for (struct idstr *e = device->config_list.next ;
       e != &device->config_list ;
       e = e->next) {
    switch (i) {
      case 0: fprintf(f, "%s", e->value); break;
      case 1: fprintf(f, " %s", e->value); break;
      default: fprintf(f, ",%s", e->value); break;
    }
    i++;
  }
  fprintf(f, "\n");
  
  return MOBERG_OK;
}

static struct moberg_status stop(struct moberg_device_context *device,
                                 FILE *f)
{
  fprintf(f, "comedi_config --remove %s\n", device->name);
  for (struct idstr *e = device->modprobe_list.prev ;
       e != &device->modprobe_list ;
       e = e->prev) {
    fprintf(f, "rmmod %s\n", e->value);
  }
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
