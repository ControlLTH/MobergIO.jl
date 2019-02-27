#include <stdlib.h>
#include <string.h>
#include <moberg_config.h>

struct moberg_config
{
  struct device_entry {
    struct device_entry *next;
    struct moberg_device *device;
  } *device_head, **device_tail;
};

struct moberg_config *moberg_config_new()
{
  struct moberg_config *result = malloc(sizeof *result);

  if (result) {
    result->device_head = NULL;
    result->device_tail = &result->device_head;
  }
  
  return result;
}

void moberg_config_free(struct moberg_config *config)
{
  struct device_entry *entry = config->device_head;
  while (entry) {
    struct device_entry *tmp = entry;
    entry = entry->next;
    moberg_device_free(tmp->device);
    free(tmp);
  }
  free(config);
}

int moberg_config_join(struct moberg_config *dest,
                       struct moberg_config *src)
{
  if (src && dest) {
    while (src->device_head) {
      struct device_entry *d = src->device_head;
      src->device_head = d->next;
      if (! src->device_head) {
        src->device_tail = &src->device_head;
      }
        
      *dest->device_tail = d;
      dest->device_tail = &d->next;
    }
    return 1;
  }
  return 0;
}

int moberg_config_add_device(struct moberg_config *config,
                             struct moberg_device *device)
{
  struct device_entry *entry = malloc(sizeof(*entry));

  if (! entry) { goto err; }
  entry->next = NULL;
  entry->device = device;
  *config->device_tail = entry;
  config->device_tail = &entry->next;

  return 1;
err:
  return 0;
}

int moberg_config_install_channels(struct moberg_config *config,
                                   struct moberg_channel_install *install)
{
  int result = 1;
  for (struct device_entry *d = config->device_head ; d ; d = d->next) {
    result &= moberg_device_install_channels(d->device, install);
  }
  return result;
}


