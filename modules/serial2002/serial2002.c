#include <moberg_config.h>
#include <moberg_parser.h>
#include <moberg_module.h>
#include <moberg_device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum moberg_parser_token_kind kind_t;
typedef struct moberg_parser_token token_t;
typedef struct moberg_parser_context context_t;

struct moberg_device_context {
  int (*dlclose)(void *dlhandle);
  void *dlhandle;
  int use_count;
  char *name;
  int baud;
};

struct moberg_channel_context {
  void *to_free; /* To be free'd when use_count goes to zero */
  struct moberg_device_context *device;
  int use_count;
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

static struct moberg_device_context *new_context(
  int (*dlclose)(void *dlhandle),
  void *dlhandle)
{
  struct moberg_device_context *result = malloc(sizeof(*result));
  if (result) {
    memset(result, 0, sizeof(*result));
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
    moberg_deferred_action(context->dlclose, context->dlhandle);
    free(context->name);
    free(context);
    return 0;
  }
  return context->use_count;
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

static void init_channel(
  struct moberg_channel *channel,
  void *to_free,
  struct moberg_channel_context *context,
  struct moberg_device_context *device,  
  enum moberg_channel_kind kind,
  union moberg_channel_action action)
{
  context->to_free = to_free;
  context->device = device;
  context->use_count = 0;

  channel->context = context;
  channel->up = channel_up;
  channel->down = channel_down;
  channel->open = NULL;
  channel->close = NULL;
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
  
static int parse_config(
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
      device->name = strndup(name.u.string.value, name.u.string.length);
    } else if (acceptkeyword(c, "baud")) {
      token_t baud;
      if (! acceptsym(c, tok_EQUAL, NULL)) { goto syntax_err; }
      if (! acceptsym(c, tok_INTEGER, &baud)) { goto syntax_err; }
      if (! acceptsym(c, tok_SEMICOLON, NULL)) { goto syntax_err; }
      device->baud = baud.u.integer.value;
    } else {
      goto syntax_err;
    }
  }
  return 1;
syntax_err:
  moberg_parser_failed(c, stderr);
  return 0;
}

static int parse_map(
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
        struct moberg_channel_analog_in *channel =
          malloc(sizeof(*channel));
        
        if (channel) {
          init_channel(&channel->channel,
                       channel,
                       &channel->channel_context,
                       device,
                       kind,
                       (union moberg_channel_action) {
                         .analog_in.context=channel,
                         .analog_in.read=NULL });
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
                       kind,
                       (union moberg_channel_action) {
                         .encoder_in.context=channel,
                         .encoder_in.read=NULL });
          map->map(map->device, &channel->channel);
        }
      } break;
    }
  }
  return 1;
syntax_err:
  moberg_parser_failed(c, stderr);
  return 0;
}

struct moberg_device_driver moberg_device_driver = {
  .new=new_context,
  .up=device_up,
  .down=device_down,
  .parse_config=parse_config,
  .parse_map=parse_map,
};
