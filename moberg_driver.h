#ifndef __MOBERG_DRIVER_H__
#define __MOBERG_DRIVER_H__

#include <moberg_config_parser.h>

struct moberg_driver_config {
};

struct moberg_driver_map {
};

typedef struct moberg_driver_config (*moberg_driver_parse_config_t)(
  struct moberg_config_parser_context *context);

typedef struct moberg_driver_map (*moberg_driver_parse_map_t)(
  struct moberg_config_parser_context *context,
  enum moberg_config_parser_token_kind kind);

struct moberg_driver {
  void *handle;
  struct moberg_driver_module {
    moberg_driver_parse_config_t parse_config;
    moberg_driver_parse_map_t parse_map;
  } module;
};

struct moberg_driver *moberg_open_driver(struct moberg_config_parser_ident id);

void moberg_close_driver(struct moberg_driver *driver);


#endif
