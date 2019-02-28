#include <moberg_config.h>
#include <moberg_parser.h>
#include <moberg_module.h>
#include <moberg_device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comedilib.h>

typedef enum moberg_parser_token_kind kind_t;
typedef struct moberg_parser_token token_t;
typedef struct moberg_parser_ident ident_t;
typedef struct moberg_parser_context context_t;

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

static int analog_in_read(struct moberg_channel_analog_in *analog_in,
                          double *value)
{
  struct channel_descriptor descriptor = analog_in->channel_context.descriptor;
  lsampl_t data;
  comedi_data_read(analog_in->channel_context.device->comedi.handle,
                   descriptor.subdevice,
                   descriptor.subchannel,
                   0, 0, &data);
        
  *value = descriptor.min + data * descriptor.delta;
  fprintf(stderr, "Data: %d %f\n", data, *value);
  return 1;
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

static int device_open(struct moberg_device_context *device)
{
  if (device->comedi.count == 0) {
    device->comedi.handle = comedi_open(device->name);
    if (device->comedi.handle == NULL) {
      goto err;
    }
  }
  device->comedi.count++;
  return 1;
err:
  fprintf(stderr, "Failed to open %s\n", device->name);
  return 0;
}

static int device_close(struct moberg_device_context *device)
{
  device->comedi.count--;
  if (device->comedi.count == 0) {
    comedi_close(device->comedi.handle);
  }

  return 1;
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

static int channel_open(struct moberg_channel *channel)
{
  channel_up(channel);
  if (! device_open(channel->context->device)) { goto err; }
  fprintf(stderr, "Open %s[%d][%d]\n",
          channel->context->device->name, 
          channel->context->descriptor.subdevice, 
          channel->context->descriptor.subchannel);
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
    goto err;
  }
  if (! range) {
    fprintf(stderr, "Failed to get range for %s[%d][%d]\n",
            channel->context->device->name, 
            channel->context->descriptor.subdevice, 
            channel->context->descriptor.subchannel);
    goto err;
  }
  channel->context->descriptor.maxdata = maxdata;
  channel->context->descriptor.min = range->min;
  channel->context->descriptor.max = range->max;
  channel->context->descriptor.delta = (range->max - range->min) / maxdata;
  return 1;
err:
  return 0;
}

static int channel_close(struct moberg_channel *channel)
{
  device_close(channel->context->device);
  channel_down(channel);
  return 1;  
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

static inline int acceptsym(context_t *c,
			   kind_t kind,
			   token_t *token)
{
  return moberg_parser_acceptsym(c, kind, token);
}
  
static inline int acceptkeyword(context_t *c,
				const char *keyword)
{
  return moberg_parser_acceptkeyword(c, keyword);
}

static int append_modprobe(struct moberg_device_context *device,
                           token_t token)
{
  struct idstr *modprobe = malloc(sizeof(*modprobe));
  if (! modprobe) { goto err; }
  modprobe->value = strndup(token.u.idstr.value, token.u.idstr.length);
  if (! modprobe->value) { goto free_modprobe; }
  modprobe->prev = device->modprobe_list.prev;
  modprobe->next = modprobe->prev->next;
  modprobe->prev->next = modprobe;
  modprobe->next->prev = modprobe;
  return 1;
free_modprobe:
  free(modprobe);
err:
  return 0;
}
  
static int append_config(struct moberg_device_context *device,
                           token_t token)
{
  struct idstr *config = malloc(sizeof(*config));
  if (! config) { goto err; }
  config->value = strndup(token.u.idstr.value, token.u.idstr.length);
  if (! config->value) { goto free_config; }
  config->prev = device->config_list.prev;
  config->next = config->prev->next;
  config->prev->next = config;
  config->next->prev = config;
  return 1;
free_config:
  free(config);
err:
  return 0;
}
  
static int parse_config(struct moberg_device_context *device,
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
  return 1;
syntax_err:
  moberg_parser_failed(c, stderr);
  return 0;
}

static int parse_map(struct moberg_device_context *device,
                     struct moberg_parser_context *c,
                     enum moberg_channel_kind kind,
                     struct moberg_channel_map *map)
{
  token_t min, max, route={ .u.integer.value=0 };

  if (! acceptsym(c, tok_LBRACE, NULL)) { goto err; }
  for (;;) {
    token_t subdevice;
    if (! acceptkeyword(c, "subdevice") != 0) {  goto err; }
    if (! acceptsym(c, tok_LBRACKET, NULL)) { goto err; }
    if (! acceptsym(c, tok_INTEGER, &subdevice)) { goto err; }
    if (! acceptsym(c, tok_RBRACKET, NULL)) { goto err; }
    if (acceptkeyword(c, "route")) {
      if (! acceptsym(c, tok_INTEGER, &route)) { goto err; }
    }
    if (! acceptsym(c, tok_LBRACKET, NULL)) { goto err; }
    if (! acceptsym(c, tok_INTEGER, &min)) { goto err; }
    if (acceptsym(c, tok_COLON, NULL)) { 
      if (! acceptsym(c, tok_INTEGER, &max)) { goto err; }
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
          struct moberg_channel_analog_in *channel =
            malloc(sizeof(*channel));
          
          if (channel) {
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
          }
        } break;
        case chan_ANALOGOUT: {
          struct moberg_channel_analog_out *channel =
            malloc(sizeof(*channel));
          
          if (channel) {
            init_channel(&channel->channel,
                         channel,
                         &channel->channel_context,
                         device,
                         descriptor,
                         kind,
                         (union moberg_channel_action) {
                           .analog_out.context=channel,
                           .analog_out.write=NULL });
            map->map(map->device, &channel->channel);
          }
        } break;
        case chan_DIGITALIN: {
          struct moberg_channel_digital_in *channel =
            malloc(sizeof(*channel));
          
          if (channel) {
            init_channel(&channel->channel,
                         channel,
                         &channel->channel_context,
                         device,
                         descriptor,
                         kind,
                         (union moberg_channel_action) {
                           .digital_in.context=channel,
                           .digital_in.read=NULL });
            map->map(map->device, &channel->channel);
          }
        } break;
        case chan_DIGITALOUT: {
          struct moberg_channel_digital_out *channel =
            malloc(sizeof(*channel));
          
          if (channel) {
            init_channel(&channel->channel,
                         channel,
                         &channel->channel_context,
                         device,
                         descriptor,
                         kind,
                         (union moberg_channel_action) {
                           .digital_out.context=channel,
                           .digital_out.write=NULL });
            map->map(map->device, &channel->channel);
          }
        } break;
        case chan_ENCODERIN: {
          struct moberg_channel_encoder_in *channel =
            malloc(sizeof(*channel));
          
          if (channel) {
            init_channel(&channel->channel,
                         channel,
                         &channel->channel_context,
                         device,
                         descriptor,
                         kind,
                         (union moberg_channel_action) {
                           .encoder_in.context=channel,
                           .encoder_in.read=NULL });
            map->map(map->device, &channel->channel);
          }
        } break;
      }
    }
    if (! acceptsym(c, tok_RBRACKET, NULL)) { goto err; }
    if (! acceptsym(c, tok_COMMA, NULL)) { break; }
    
  }
  if (! acceptsym(c, tok_RBRACE, NULL)) { goto err; }
  return 1;
err:
  moberg_parser_failed(c, stderr);
  return 0;
}

static int start(struct moberg_device_context *device,
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
  
  return 1;
}

static int stop(struct moberg_device_context *device,
                 FILE *f)
{
  fprintf(f, "comedi_config --remove %s\n", device->name);
  for (struct idstr *e = device->modprobe_list.prev ;
       e != &device->modprobe_list ;
       e = e->prev) {
    fprintf(f, "rmmod %s\n", e->value);
  }
  return 1;
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
