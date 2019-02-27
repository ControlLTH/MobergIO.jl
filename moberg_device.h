#ifndef __MOBERG_DEVICE_H__
#define __MOBERG_DEVICE_H__

struct moberg_device;
struct moberg_device_context;

#include <moberg.h>
#include <moberg_config.h>
#include <moberg_parser.h>
#include <moberg_channel.h>

struct moberg_parser_context;

struct moberg_device_driver {
  /* Create new device context */
  struct moberg_device_context *(*new)(int (*dlclose)(void *dlhandle),
                                       void *dlhandle);
  /* Use-count of device, when it reaches zero, device will be free'd */
  int (*up)(struct moberg_device_context *context);
  int (*down)(struct moberg_device_context *context);
  /* Parse driver dependent parts of config file */
  int (*parse_config)(
    struct moberg_device_context *device,
    struct moberg_parser_context *parser);
  int (*parse_map)(
    struct moberg_device_context *device,
    struct moberg_parser_context *parser,
    enum moberg_channel_kind kind,
    struct moberg_channel_map *map);
};

struct moberg_device;

struct moberg_device *moberg_device_new(const char *driver);

void moberg_device_free(struct moberg_device *device);

int moberg_device_parse_config(struct moberg_device* device,
                               struct moberg_parser_context *context);

int moberg_device_parse_map(struct moberg_device* device,
                            struct moberg_parser_context *parser,
                            enum moberg_channel_kind kind,
                            int min,
                            int max);

int moberg_device_install_channels(struct moberg_device *device,
                                   struct moberg_channel_install *install);


#endif
