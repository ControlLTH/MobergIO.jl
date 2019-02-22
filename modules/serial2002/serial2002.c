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
  
struct moberg_device_config {
  char *device;
  int baud;
};

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
    } else if (acceptkeyword(c, "baud")) {
      token_t baud;
      if (! acceptsym(c, tok_EQUAL, NULL)) { goto syntax_err; }
      if (! acceptsym(c, tok_INTEGER, &baud)) { goto syntax_err; }
      if (! acceptsym(c, tok_SEMICOLON, NULL)) { goto syntax_err; }
      config->baud = baud.u.integer.value;
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
  enum moberg_channel_kind ignore)
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
  return 1;
syntax_err:
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
