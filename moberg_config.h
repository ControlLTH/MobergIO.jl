#ifndef __MOBERG_CONFIG_H__
#define __MOBERG_CONFIG_H__

#include <moberg.h>
#include <moberg_device.h>

struct moberg_config *moberg_config_new();

void moberg_config_free(struct moberg_config *config);

int moberg_config_join(struct moberg_config *dest,
                       struct moberg_config *src);

int moberg_config_add_device(struct moberg_config *config,
                             struct moberg_device *device);

int moberg_config_install_channels(struct moberg_config *config,
                                   struct moberg_channel_install *install);

int moberg_config_start(struct moberg_config *config,
                        FILE *f);

int moberg_config_stop(struct moberg_config *config,
                       FILE *f);

#endif
