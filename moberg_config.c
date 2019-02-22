#include <stdlib.h>
#include <string.h>
#include <moberg_config.h>

#define LIST_COND_GROW(LIST, INDEX, ONERR)                                \
  ({                                                                    \
    if (LIST.capacity <= INDEX) {                                       \
      int new_cap;                                                      \
      for (new_cap = 2 ; new_cap <= INDEX ; new_cap *= 2);              \
      void *new = realloc(LIST.value, new_cap * sizeof(*LIST.value));   \
      if (! new) { ONERR; }                                             \
      void *p = new + LIST.capacity * sizeof(*LIST.value);              \
      memset(p, 0, (new_cap - LIST.capacity) * sizeof(*LIST.value));    \
      LIST.value = new;                                                 \
      LIST.capacity= new_cap;                                           \
    }                                                                   \
  })

#define LIST_SET(LIST, INDEX, VALUE, ONERR)       \
  ( LIST_COND_GROW(LIST, INDEX, ONERR),           \
    LIST.value[INDEX] = VALUE )

#define LIST_GET(LIST, INDEX, VALUE, ONERR)       \
  ( LIST_COND_GROW(LIST, INDEX, ONERR),           \
    LIST.value[INDEX] )

struct moberg_config
{
  struct device_entry {
    struct device_entry *next;
    struct moberg_device *device;
  } *device;
  struct  {
    int capacity;
    struct analog_in_entry {
      struct moberg_device* device;
      struct moberg_device_analog_in *channel;
    } *value;
  } analog_in;
};

struct moberg_config *moberg_config_new()
{
  struct moberg_config *result = malloc(sizeof *result);

  result->device = NULL;
  
  return result;
}

void moberg_config_free(struct moberg_config *config)
{
  struct device_entry *entry = config->device;
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
    struct device_entry **tail;
    for (tail = &dest->device ; *tail ; tail = &(*tail)->next) { }
    *tail = src->device;
    src->device = NULL;
    return 1;
  }
  return 0;
}

int moberg_config_add_device(struct moberg_config *config,
                             struct moberg_device *device)
{
  struct device_entry *entry = malloc(sizeof(*entry));

  entry->next = config->device;
  entry->device = device;
  config->device = entry;

  return 1;
}

int moberg_config_add_analog_in(struct moberg_config *config,
                                int index,
                                struct moberg_device* device,
                                struct moberg_device_analog_in *channel)
{
  struct analog_in_entry e = { device, channel };
  LIST_SET(config->analog_in, index, e, goto err);
  return 1;
err:
  return 0;
}
                            
