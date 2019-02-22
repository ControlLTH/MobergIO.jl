#ifndef __MOBERG_CONFIG_H__
#define __MOBERG_CONFIG_H__

#include <moberg_device.h>

struct moberg_config;

struct moberg_config *moberg_config_new();

void moberg_config_free(struct moberg_config *config);

int moberg_config_join(struct moberg_config *dest,
                       struct moberg_config *src);

int moberg_config_add_device(struct moberg_config *config,
                             struct moberg_device *device);

int moberg_config_add_analog_in(struct moberg_config *config,
                                int index,
                                struct moberg_device* device,
                                struct moberg_device_analog_in *channel);
                            
int moberg_config_add_analog_out(struct moberg_config *config,
                                 int index,
                                 struct moberg_device* device,
                                 struct moberg_device_analog_out *channel);
                            
int moberg_config_add_digital_in(struct moberg_config *config,
                                 int index,
                                 struct moberg_device* device,
                                 struct moberg_device_digital_in *channel);
                            
int moberg_config_add_digital_out(struct moberg_config *config,
                                  int index,
                                  struct moberg_device* device,
                                  struct moberg_device_digital_out *channel);
                            
int moberg_config_add_encoder_in(struct moberg_config *config,
                                 int index,
                                 struct moberg_device* device,
                                 struct moberg_device_encoder_in *channel);



#endif
