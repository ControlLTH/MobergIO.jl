#ifndef __MOBERG_DEVICE_H__
#define __MOBERG_DEVICE_H__

struct moberg_device {
  int (*add_config)(void *config);
  int (*add_map)(int config);
};

struct moberg_device *moberg_device_new(struct moberg_driver *driver);

void moberg_device_free(struct moberg_device *device);

#endif
