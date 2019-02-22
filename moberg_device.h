#ifndef __MOBERG_DEVICE_H__
#define __MOBERG_DEVICE_H__

#include <moberg_config.h>
#include <moberg_parser.h>

struct moberg_device_map_range {
  enum moberg_device_map_kind {
    map_analog_in,
    map_analog_out,
    map_digital_in,
    map_digital_out,
    map_encoder_in
  } kind;
  int min;
  int max;
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
    enum moberg_device_map_kind kind);
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
                            struct moberg_parser_context *context,
                            struct moberg_device_map_range range);

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

#endif
