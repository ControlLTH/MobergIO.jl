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

struct moberg_device_map {};
struct moberg_device_config {
  char *device;
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
  struct moberg_device *device,
  struct moberg_parser_context *c)
{
  struct moberg_device_config *config = malloc(sizeof *config);
  if (! config) {
    fprintf(stderr, "Failed to allocate moberg device config\n");
    goto err;
  }
  if (! acceptsym(c, tok_LBRACE, NULL)) { goto syntax_err; }
  for (;;) {
    if (acceptsym(c, tok_RBRACE, NULL)) {
	break;
    } else if (acceptkeyword(c, "device")) {
      token_t device;
      if (! acceptsym(c, tok_EQUAL, NULL)) { goto syntax_err; }
      if (! acceptsym(c, tok_STRING, &device)) { goto syntax_err; }
      if (! acceptsym(c, tok_SEMICOLON, NULL)) { goto syntax_err; }
      config->device = strndup(device.u.string.value, device.u.string.length);
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
  moberg_device_set_config(device, config);
  return 1;
syntax_err:
  moberg_parser_failed(c, stderr);
  free(config);
err:
  return 0;
}

static int parse_map(
  struct moberg_device *device,
  struct moberg_parser_context *c,
  enum moberg_channel_kind kind)
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
        case chan_ANALOGIN:
          moberg_device_add_analog_in(device, NULL);
          break;
        case chan_ANALOGOUT:
          moberg_device_add_analog_out(device, NULL);
          break;
        case chan_DIGITALIN:
          moberg_device_add_digital_in(device, NULL);
          break;
        case chan_DIGITALOUT:
          moberg_device_add_digital_out(device, NULL);
          break;
        case chan_ENCODERIN:
          moberg_device_add_encoder_in(device, NULL);
          break;
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

static int config_free(struct moberg_device_config *config)
{
  free(config->device);
  return 1;
}

struct moberg_device_driver moberg_device_driver = {
  .parse_config = parse_config,
  .parse_map = parse_map,
  .config_free = config_free
};
