#include <moberg_config_parser_module.h>
#include <moberg_driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum moberg_config_parser_token_kind kind_t;
typedef struct moberg_config_parser_token token_t;
typedef struct moberg_config_parser_ident ident_t;
typedef struct moberg_config_parser_context context_t;

static inline int acceptsym(context_t *c,
			   kind_t kind,
			   token_t *token)
{
  return moberg_config_parser_acceptsym(c, kind, token);
}
  
static inline int acceptkeyword(context_t *c,
				const char *keyword)
{
  return moberg_config_parser_acceptkeyword(c, keyword);
}
  
struct moberg_driver_device {
  const char *device;
};


static struct moberg_driver_device *parse_config(
  struct moberg_config_parser_context *c)
{
  struct moberg_driver_device *result = malloc(sizeof *result);
  if (! result) {
    fprintf(stderr, "Failed to allocate moberg device\n");
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
      result->device = strndup(device.u.string.value, device.u.string.length);
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
  return result;
syntax_err:
  moberg_config_parser_failed(c, stderr);
err:
  return NULL;
}

static struct moberg_driver_map parse_map(
  struct moberg_config_parser_context *c,
  enum moberg_config_parser_token_kind kind)
{
  struct moberg_driver_map result;

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
    if (! acceptsym(c, tok_INTEGER, NULL)) { goto err; }
    if (acceptsym(c, tok_COLON, NULL)) { 
      if (! acceptsym(c, tok_INTEGER, NULL)) { goto err; }
    }
    if (! acceptsym(c, tok_RBRACKET, NULL)) { goto err; }
    if (! acceptsym(c, tok_COMMA, NULL)) { break; }
  }
  if (! acceptsym(c, tok_RBRACE, NULL)) { goto err; }
  return result;
err:
  moberg_config_parser_failed(c, stderr);
  exit(1);
}

struct moberg_driver_module moberg_module = {
  .parse_config = parse_config,
  .parse_map = parse_map,
};
