#include <moberg_config_parser.h>
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
  

static struct moberg_driver_config parse_config(
  struct moberg_config_parser_context *c)
{
  struct moberg_driver_config result;
  token_t t;
  printf("PARSE_CONFIG %s\n", __FILE__);
  printf("LBRACE %d", acceptsym(c, tok_LBRACE, &t));
  for (;;) {
    if (acceptsym(c, tok_RBRACE, NULL)) {
	break;
    } else if (acceptkeyword(c, "device")) {
      token_t device;
      if (! acceptsym(c, tok_EQUAL, NULL)) { goto err; }
      if (! acceptsym(c, tok_STRING, &device)) { goto err; }
      if (! acceptsym(c, tok_SEMICOLON, NULL)) { goto err; }
    } else if (acceptkeyword(c, "config")) {
      if (! acceptsym(c, tok_EQUAL, NULL)) { goto err; }
      if (! acceptsym(c, tok_LBRACKET, NULL)) { goto err; }
      for (;;) {
	if (acceptsym(c, tok_RBRACKET, NULL)) {
	  break;
	} else if (acceptsym(c, tok_IDENT, NULL) ||
		   acceptsym(c, tok_STRING, NULL)) {
	} else {
	  goto err;
	}
      }
      if (! acceptsym(c, tok_SEMICOLON, NULL)) { goto err; }
    } else if (acceptkeyword(c, "modprobe")) {
      if (! acceptsym(c, tok_EQUAL, NULL)) { goto err; }
      if (! acceptsym(c, tok_LBRACKET, NULL)) { goto err; }
      for (;;) {
	if (acceptsym(c, tok_RBRACKET, NULL)) {
	  break;
	} else if (acceptsym(c, tok_IDENT, NULL) ||
		   acceptsym(c, tok_STRING, NULL)) {
	} else {
	  goto err;
	}
      }
      if (! acceptsym(c, tok_SEMICOLON, NULL)) { goto err; }
    } else {
      goto err;
    }
  }
  printf("PARSE_CONFIG DONE%s\n", __FILE__);
  return result;
 err:
  moberg_config_parser_failed(c, stderr);
  exit(1);
}

static struct moberg_driver_map parse_map(
  struct moberg_config_parser_context *c,
  enum moberg_config_parser_token_kind kind)
{
  struct moberg_driver_map result;

  token_t t;
  printf("PARSE_MAP %s\n", __FILE__);
  if (! acceptkeyword(c, "subdevice") != 0) {  goto err; }
  if (! acceptsym(c, tok_LBRACKET, NULL)) { goto err; }
  if (! acceptsym(c, tok_INTEGER, &t)) { goto err; }
  if (! acceptsym(c, tok_RBRACKET, NULL)) { goto err; }
    
/*
  const char *buf = context->buf;
  while (*buf && *buf != '}') {
    buf++;
  }
  context->buf = buf + 1;
*/
  return result;
err:
  moberg_config_parser_failed(c, stderr);
  exit(1);
}

struct moberg_driver_module moberg_module = {
  .parse_config = parse_config,
  .parse_map = parse_map,
};
