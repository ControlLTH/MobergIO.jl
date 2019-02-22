#ifndef __MOBERG_DEVICE_H__
#define __MOBERG_DEVICE_H__

#include <moberg.h>
#include <moberg_config.h>
#include <moberg_parser.h>

enum moberg_channel_kind {
    chan_ANALOGIN,
    chan_ANALOGOUT,
    chan_DIGITALIN,
    chan_DIGITALOUT,
    chan_ENCODERIN
};

struct moberg_device_analog_in {
  struct moberg_device_analog_in_context *context;
  int (*open)(struct moberg_device_analog_in_context *);
  int (*close)(struct moberg_device_analog_in_context *);
  int (*read)(struct moberg_device_analog_in_context *, double *value);
};
  
struct moberg_device_analog_out {
  struct moberg_device_analog_out_context *context;
  int (*open)(struct moberg_device_analog_out_context *);
  int (*close)(struct moberg_device_analog_out_context *);
  int (*write)(struct moberg_device_analog_out_context *, double value);
};
  
struct moberg_device_digital_in {
  struct moberg_device_digital_in_context *context;
  int (*open)(struct moberg_device_digital_in_context *);
  int (*close)(struct moberg_device_digital_in_context *);
  int (*read)(struct moberg_device_digital_in_context *, int *value);
};
  
struct moberg_device_digital_out {
  struct moberg_device_digital_out_context *context;
  int (*open)(struct moberg_device_digital_out_context *);
  int (*close)(struct moberg_device_digital_out_context *);
  int (*write)(struct moberg_device_digital_out_context *, int value);
};
  
struct moberg_device_encoder_in {
  struct moberg_device_encoder_in_context *context;
  int (*open)(struct moberg_device_encoder_in_context *);
  int (*close)(struct moberg_device_encoder_in_context *);
  int (*read)(struct moberg_device_encoder_in_context *, long *value);
};
  
struct moberg_install_channels {
  struct moberg *context;
  int (*analog_in)(int index, struct moberg_device_analog_in *channel);
  int (*analog_out)(int index, struct moberg_device_analog_out *channel);
  int (*digital_in)(int index, struct moberg_device_digital_in *channel);
  int (*digital_out)(int index, struct moberg_device_digital_in *channel);
  int (*encoder_in)(int index, struct moberg_device_encoder_in *channel);
};

struct moberg_parser_context;
struct moberg_device;
struct moberg_device_config;

struct moberg_device_driver {
  int (*parse_config)(
    struct moberg_device* device,
    struct moberg_parser_context *context);
  int (*parse_map)(
    struct moberg_device* device,
    struct moberg_parser_context *context,
    enum moberg_channel_kind kind);
  int (*config_free)(
    struct moberg_device_config *config);
};

struct moberg_device;

struct moberg_device *moberg_device_new(const char *driver);

void moberg_device_free(struct moberg_device *device);

int moberg_device_parse_config(struct moberg_device* device,
                               struct moberg_parser_context *context);

int moberg_device_set_config(struct moberg_device* device,
                             struct moberg_device_config *config);

int moberg_device_parse_map(struct moberg_device* device,
                            struct moberg_config *config,
                            struct moberg_parser_context *context,
                            enum moberg_channel_kind kind,
                            int min,
                            int max);

int moberg_device_add_analog_in(struct moberg_device* device,
                                struct moberg_device_analog_in *channel);
                            
int moberg_device_add_analog_out(struct moberg_device* device,
                                 struct moberg_device_analog_out *channel);
                            
int moberg_device_add_digital_in(struct moberg_device* device,
                                 struct moberg_device_digital_in *channel);
                            
int moberg_device_add_digital_out(struct moberg_device* device,
                                  struct moberg_device_digital_out *channel);
                            
int moberg_device_add_encoder_in(struct moberg_device* device,
                                 struct moberg_device_encoder_in *channel);

int moberg_device_install_channels(struct moberg_device *device,
                                   struct moberg_install_channels *install);


#endif
