#include <moberg_config.h>
#include <moberg_parser.h>
#include <moberg_module.h>
#include <moberg_device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum moberg_parser_token_kind kind_t;
typedef struct moberg_parser_token token_t;
typedef struct moberg_parser_ident ident_t;
typedef struct moberg_parser_context context_t;

struct moberg_device_context {
  int (*dlclose)(void *dlhandle);
  void *dlhandle;
  int use_count;
  char *device;
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
  int result = context->use_count;
  if (context->use_count <= 0) {
    moberg_deferred_action(context->dlclose, context->dlhandle);
    free(context->device);
    free(context);
    return 0;
  }
  return result;
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
      device->device = strndup(name.u.string.value, name.u.string.length);
    } else if (acceptkeyword(c, "config")) {
      if (! acceptsym(c, tok_EQUAL, NULL)) { goto syntax_err; }
      if (! acceptsym(c, tok_LBRACKET, NULL)) { goto syntax_err; }
      for (;;) {
	if (acceptsym(c, tok_RBRACKET, NULL)) {
	  break;
	} else if (acceptsym(c, tok_IDENT, NULL) ||
		   acceptsym(c, tok_STRING, NULL)) {
	} else {
	  goto syntax_err;
	}
      }
      if (! acceptsym(c, tok_SEMICOLON, NULL)) { goto syntax_err; }
    } else if (acceptkeyword(c, "modprobe")) {
      if (! acceptsym(c, tok_EQUAL, NULL)) { goto syntax_err; }
      if (! acceptsym(c, tok_LBRACKET, NULL)) { goto syntax_err; }
      for (;;) {
	if (acceptsym(c, tok_RBRACKET, NULL)) {
	  break;
	} else if (acceptsym(c, tok_IDENT, NULL) ||
		   acceptsym(c, tok_STRING, NULL)) {
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

static int parse_map(
  struct moberg_device_context *device,
  struct moberg_parser_context *c,
  enum moberg_channel_kind kind,
  struct moberg_channel_map *map)
{
  token_t min, max;

  if (! acceptsym(c, tok_LBRACE, NULL)) { goto err; }
  for (;;) {
    token_t subdevice;
    if (! acceptkeyword(c, "subdevice") != 0) {  goto err; }
    if (! acceptsym(c, tok_LBRACKET, NULL)) { goto err; }
    if (! acceptsym(c, tok_INTEGER, &subdevice)) { goto err; }
    if (! acceptsym(c, tok_RBRACKET, NULL)) { goto err; }
    if (acceptkeyword(c, "route")) {
      token_t route;
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
    if (! acceptsym(c, tok_RBRACKET, NULL)) { goto err; }
    if (! acceptsym(c, tok_COMMA, NULL)) { break; }
    
  }
  if (! acceptsym(c, tok_RBRACE, NULL)) { goto err; }
  return 1;
err:
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
