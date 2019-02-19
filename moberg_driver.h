#ifndef __MOBERG_DRIVER_H__
#define __MOBERG_DRIVER_H__

#include <moberg_config_parser.h>

struct moberg_driver_device;

struct moberg_driver_map {
};

typedef struct moberg_driver_device *(*moberg_driver_parse_config_t)(
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

struct moberg_driver *moberg_driver_open(struct moberg_config_parser_ident id);

void moberg_driver_close(struct moberg_driver *driver);

void moberg_driver_device_free(struct moberg_driver_device *device);


#endif
