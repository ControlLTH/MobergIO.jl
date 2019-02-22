#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <moberg_config.h>
#include <moberg_device.h>

struct moberg_device {
  void *driver_handle;
  struct moberg_device_driver driver;
  struct moberg_device_config *device_config;
  struct channel_list {
    struct channel_list *next;
    enum moberg_channel_kind kind;
    int index;
    union channel {
      struct moberg_device_analog_in *analog_in;
      struct moberg_device_analog_out *analog_out;
      struct moberg_device_digital_in *digital_in;
      struct moberg_device_digital_out *digital_out;
      struct moberg_device_encoder_in *encoder_in;
    } u;
  } *channel_head, **channel_tail;
  struct map_range {
    struct moberg_config *config;
    enum moberg_channel_kind kind;
    int min;
    int max;
  } *range;
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
  result->device_config = NULL;
  result->channel_head = NULL;
  result->channel_tail = &result->channel_head;
  result->range = NULL;
  
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
  struct channel_list *channel = device->channel_head;
  while (channel) {
    struct channel_list *next;
    next = channel->next;
    free(channel);
    channel = next;
  }
  device->driver.config_free(device->device_config);
  free(device->device_config);
  dlclose(device->driver_handle);
  free(device);
}

int moberg_device_parse_config(struct moberg_device *device,
                               struct moberg_parser_context *context)
{
  return device->driver.parse_config(device, context);
}

int moberg_device_set_config(struct moberg_device *device,
                             struct moberg_device_config *config)
{
  if (device->device_config) {
    device->driver.config_free(device->device_config);
    free(device->device_config);
  }
  device->device_config = config;
  return 1;
}

int moberg_device_parse_map(struct moberg_device* device,
                            struct moberg_config *config,
                            struct moberg_parser_context *context,
                            enum moberg_channel_kind kind,
                            int min,
                            int max)
{
  int result;
  struct map_range r = {
    .config=config,
    .kind=kind,
    .min=min,
    .max=max
  };
  device->range = &r;
  result = device->driver.parse_map(device, context, kind);
  device->range = NULL;
  printf("RRR %d %d\n", r.min, r.max);
  return result;
}

static int add_channel(struct moberg_device* device,
                       enum moberg_channel_kind kind,
                       int index,
                       union channel channel)
{
  struct channel_list *element = malloc(sizeof(*element));
  if (! element) { goto err; }
  element->next = NULL;
  element->kind = kind;
  element->index = index;
  element->u = channel;
  *device->channel_tail = element;
  device->channel_tail = &element->next;
  return 1;
err:
  return 0;
}

int moberg_device_add_analog_in(struct moberg_device* device,
                                struct moberg_device_analog_in *channel)
{
  int result = 0;
  
  if (device->range->kind == chan_ANALOGIN &&
      device->range->min <= device->range->max) {
    printf("Mapping %d\n", device->range->min);
    result = add_channel(device, device->range->kind, device->range->min,
                         (union channel) { .analog_in=channel });
    device->range->min++;
  }
  return result;
}
                            
int moberg_device_add_analog_out(struct moberg_device* device,
                                 struct moberg_device_analog_out *channel)
{
  int result = 0;
  
  if (device->range->kind == chan_ANALOGOUT &&
      device->range->min <= device->range->max) {
    printf("Mapping %d\n", device->range->min);
    result = add_channel(device, device->range->kind, device->range->min,
                         (union channel) { .analog_out=channel });
    device->range->min++;
  }
  return result;
}
                            
int moberg_device_add_digital_in(struct moberg_device* device,
                                struct moberg_device_digital_in *channel)
{
  int result = 0;
  
  if (device->range->kind == chan_DIGITALIN &&
      device->range->min <= device->range->max) {
    printf("Mapping %d\n", device->range->min);
    result = add_channel(device, device->range->kind, device->range->min,
                         (union channel) { .digital_in=channel });
    device->range->min++;
  }
  return result;
}
                            
int moberg_device_add_digital_out(struct moberg_device* device,
                                 struct moberg_device_digital_out *channel)
{
  int result = 0;
  
  if (device->range->kind == chan_DIGITALOUT &&
      device->range->min <= device->range->max) {
    printf("Mapping %d\n", device->range->min);
    result = add_channel(device, device->range->kind, device->range->min,
                         (union channel) { .digital_out=channel });
    device->range->min++;
  }
  return result;
}
                            
int moberg_device_add_encoder_in(struct moberg_device* device,
                                struct moberg_device_encoder_in *channel)
{
  int result = 0;
  
  if (device->range->kind == chan_ENCODERIN &&
      device->range->min <= device->range->max) {
    printf("Mapping %d\n", device->range->min);
    result = add_channel(device, device->range->kind, device->range->min,
                         (union channel) { .encoder_in=channel });
    device->range->min++;
  }
  return result;
}

int moberg_device_install_channels(struct moberg_device *device,
                                   struct moberg_install_channels *install)
{
  printf("INSTALL\n");
  return 1;
}

