#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <moberg_config.h>
#include <moberg_device.h>

struct moberg_device {
  void *driver_handle;
  struct moberg_device_driver driver;
  struct moberg_device_config *config;
  struct moberg_device_map_range *range;
};

struct moberg_device *moberg_device_new(const char *driver)
{
  struct moberg_device *result = NULL;

  char *name = malloc(strlen("libmoberg_.so") + strlen(driver) + 1);
  if (!name) { goto out; }
  sprintf(name, "libmoberg_%s.so", driver);
  void *handle = dlopen(name, RTLD_LAZY || RTLD_DEEPBIND);
  if (! handle) {
    fprintf(stderr, "Could not find driver %s\n", name);
    goto free_name;
  }
  struct moberg_device_driver *device_driver =
    (struct moberg_device_driver *) dlsym(handle, "moberg_device_driver");
  if (! device_driver) {
    fprintf(stderr, "No moberg_device_driver in driver %s\n", name);
    goto dlclose_driver;
  }
  result = malloc(sizeof(*result));
  if (! result) {
    fprintf(stderr, "Could not allocate result for %s\n", name);
    goto dlclose_driver;
  }
  result->driver_handle = handle;
  result->driver = *device_driver;
  result->config = NULL;
  goto free_name;
  
dlclose_driver:
  dlclose(handle);
free_name:
  free(name);
out:
  return result;
}

void moberg_device_free(struct moberg_device *device)
{
  device->driver.config_free(device->config);
  free(device->config);
  free(device);
}

int moberg_device_parse_config(struct moberg_device *device,
                               struct moberg_config_parser_context *context)
{
  return device->driver.parse_config(device, context);
}

int moberg_device_set_config(struct moberg_device *device,
                             struct moberg_device_config *config)
{
  if (device->config) {
    device->driver.config_free(device->config);
    free(device->config);
  }
  device->config = config;
  return 1;
}

int moberg_device_parse_map(struct moberg_device* device,
                            struct moberg_config_parser_context *context,
                            struct moberg_device_map_range range)
{
  int result;
  struct moberg_device_map_range r = range;
  device->range = &r;
  result = device->driver.parse_map(device, context, range.kind);
  device->range = NULL;
  printf("RRR %d %d\n", r.min, r.max);
  return result;
}

int moberg_device_add_analog_in(struct moberg_device* device,
                                struct moberg_device_analog_in *channel)
{
  if (device->range->kind == map_analog_in &&
      device->range->min <= device->range->max) {
    printf("Mapping %d\n", device->range->min);
    // moberg_config_add_analog_in()
    device->range->min++;
    return 1;
  } else {
    return 0;
  }
}
                            
int moberg_device_add_analog_out(struct moberg_device* device,
                                 struct moberg_device_analog_out *channel)
{
  if (device->range->kind == map_analog_out &&
      device->range->min <= device->range->max) {
    printf("Mapping %d\n", device->range->min);
    device->range->min++;
    return 1;
  } else {
    return 0;
  }
}
                            
int moberg_device_add_digital_in(struct moberg_device* device,
                                 struct moberg_device_digital_in *channel)
{
  if (device->range->kind == map_digital_in &&
      device->range->min <= device->range->max) {
    printf("Mapping %d\n", device->range->min);
    device->range->min++;
    return 1;
  } else {
    return 0;
  }
}
                            
int moberg_device_add_digital_out(struct moberg_device* device,
                                  struct moberg_device_digital_out *channel)
{
  if (device->range->kind == map_digital_out &&
      device->range->min <= device->range->max) {
    printf("Mapping %d\n", device->range->min);
    device->range->min++;
    return 1;
  } else {
    return 0;
  }
}
                            
int moberg_device_add_encoder_in(struct moberg_device* device,
                                 struct moberg_device_encoder_in *channel)
{
  if (device->range->kind == map_encoder_in &&
      device->range->min <= device->range->max) {
    printf("Mapping %d\n", device->range->min);
    device->range->min++;
    return 1;
  } else {
    return 0;
  }
}

