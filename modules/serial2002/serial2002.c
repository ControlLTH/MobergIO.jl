#include <moberg_config_parser.h>
#include <moberg_driver.h>
#include <stdio.h>

static struct moberg_driver_config parse_config(
  struct moberg_config_parser_context *context)
{
  struct moberg_driver_config result;

  printf("PARSE_CONFIG %s\n", __FILE__);
/*
  const char *buf = context->buf;
  while (*buf && *buf != '}') {
    buf++;
  }
  context->buf = buf + 1;
*/
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
