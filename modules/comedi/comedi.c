#include <moberg_config_parser.h>
#include <moberg_driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define acceptsym moberg_config_parser_acceptsym
#define peeksym moberg_config_parser_peeksym

typedef struct moberg_config_parser_token token_t;
typedef struct moberg_config_parser_ident ident_t;

static struct moberg_driver_config parse_config(
  struct moberg_config_parser_context *c)
{
  struct moberg_driver_config result;
  token_t t;
  printf("PARSE_CONFIG %s\n", __FILE__);
  printf("LBRACE %d", acceptsym(c, tok_LBRACE, &t));
  for (;;) {
    if (acceptsym(c, tok_IDENT, &t) ||
	acceptsym(c, tok_CONFIG, &t)) {
      const char *v = t.u.ident.value;
      int l = t.u.ident.length;
      if (strncmp("device", v, l) == 0) {
	printf("DEVICE\n");
      } else if (strncmp("modprobe", v, l) == 0) {
	printf("MODPROBE\n");
      } else if (strncmp("comedi", v, l) == 0) {
	printf("CONFIG\n");
      }
      acceptsym(c, tok_EQUAL, NULL);
      for (;;) {
	peeksym(c, &t);
	acceptsym(c, t.kind, NULL);
	printf("%d\n", t.kind);
	if (t.kind == tok_SEMICOLON) { break; }
      }
    } else if (acceptsym(c, tok_RBRACE, NULL)) {
	break;
    } else {
      goto err;
    }
  }
  return result;
err:
  exit(1);
  return result;
}

static struct moberg_driver_map parse_map(
  struct moberg_config_parser_context *context,
  enum moberg_config_parser_token_kind kind)
{
  struct moberg_driver_map result;

  printf("PARSE_MAP %s\n", __FILE__);
/*
  const char *buf = context->buf;
  while (*buf && *buf != '}') {
    buf++;
  }
  context->buf = buf + 1;
*/
  return result;
}

struct moberg_driver_module moberg_module = {
  .parse_config = parse_config,
  .parse_map = parse_map,
};
