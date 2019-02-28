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
    struct moberg_analog_in analog_in;
    struct moberg_analog_out analog_out;
    struct moberg_digital_in digital_in;
    struct moberg_digital_out digital_out;
    struct moberg_encoder_in encoder_in;
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
