#ifndef __MOBERG_CHANNEL_H__
#define __MOBERG_CHANNEL_H__

#include <moberg.h>

enum moberg_channel_kind {
    chan_ANALOGIN,
    chan_ANALOGOUT,
    chan_DIGITALIN,
    chan_DIGITALOUT,
    chan_ENCODERIN
};

struct moberg_channel {
  struct moberg_channel_context *context;
  
  /* Use-count of channel, when it reaches zero, channel will be free'd */
  int (*up)(struct moberg_channel *channel);
  int (*down)(struct moberg_channel *channel);

  /* Channel open and close */
  int (*open)(struct moberg_channel *channel);
  int (*close)(struct moberg_channel *channel);

  /* I/O operations */
  enum moberg_channel_kind kind;
  union moberg_channel_action {
    struct {
      struct moberg_channel_analog_in *context;
      int (*read)(struct moberg_channel_analog_in *, double *value);
    } analog_in;
    struct  {
      struct moberg_channel_analog_out *context;
      int (*write)(struct moberg_channel_context *, double value);
    } analog_out;
    struct {
      struct moberg_channel_digital_in *context;
      int (*read)(struct moberg_channel_context *, int *value);
    } digital_in;
    struct  {
      struct moberg_channel_digital_out *context;
      int (*write)(struct moberg_channel_context *, int value);
    } digital_out;
    struct {
      struct moberg_channel_encoder_in *context;
      int (*read)(struct moberg_channel_context *, long *value);
    } encoder_in;
  } action;
};
  
struct moberg_channel_map {
  struct moberg_device *device;
  int (*map)(struct moberg_device* device,
             struct moberg_channel *channel);
};

struct moberg_channel_install {
  struct moberg *context;
  int (*channel)(struct moberg *context,
                 int index,
                 struct moberg_device* device,
                 struct moberg_channel *channel);
};



#endif
