#define _POSIX_C_SOURCE  200809L
#define _GNU_SOURCE               /* scandirat */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <basedir.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <moberg.h>
#include <moberg_config.h>
#include <moberg_parser.h>
#include <moberg_module.h>

struct moberg {
  struct moberg_config *config;
  struct channel_list {
    int capacity;
    struct moberg_channel **value;
  } analog_in, analog_out, digital_in, digital_out, encoder_in;
  struct deferred_action {
    struct deferred_action *next;
    int (*action)(void *param);
    void *param;
  } *deferred_action;

};

static void run_deferred_actions(struct moberg *moberg)
{
  while (moberg->deferred_action) {
    struct deferred_action *deferred = moberg->deferred_action;
    moberg->deferred_action = deferred->next;
    deferred->action(deferred->param);
    free(deferred);
  }
}

static int channel_list_set(struct channel_list *list,
                            int index,
                            struct moberg_channel *value)
{
  if (list->capacity <= index) {
    int capacity;
    for (capacity = 2 ; capacity <= index ; capacity *= 2);
    void *new = realloc(list->value, capacity * sizeof(**list->value));
    if (!new) {
      goto err;
    }
    void *p = new + list->capacity * sizeof(*list->value);
    memset(p, 0, (capacity - list->capacity) * sizeof(**list->value));
    list->value = new;
    list->capacity = capacity;
  }

  if (0 <= index && index < list->capacity) {
    list->value[index] = value;
    return 1;
  }
err:
  return 0;
}
                                   
static int channel_list_get(struct channel_list *list,
                            int index,
                            struct moberg_channel **value)
{
  if (0 <= index && index < list->capacity) {
    *value = list->value[index];
    return 1;
  }
  return 0;
}

static void channel_list_free(struct channel_list *list)
{
  for (int i = 0 ; i < list->capacity ; i++) {
    if (list->value[i]) {
      list->value[i]->down(list->value[i]);
    }
  }
  free(list->value);
}

static void parse_config_at(
  struct moberg *moberg,
  int dirfd,
  const char *pathname)
{
  if (dirfd >= 0) {
    int fd = openat(dirfd, pathname, O_RDONLY);
    if (fd >= 0) {
      struct stat statbuf;
      if (fstat(fd, &statbuf) == 0) {
        char *buf = malloc(statbuf.st_size + 1);
        if (buf) {
          if (read(fd, buf, statbuf.st_size) == statbuf.st_size) {
            buf[statbuf.st_size] = 0;
          }
          struct moberg_config *config = moberg_parse(moberg, buf);
          if (config) {
            if (! moberg->config) {
              moberg->config = config;
            } else {
              moberg_config_join(moberg->config, config);
              moberg_config_free(config);
            }
          }
          free(buf);
        }
      }
      close(fd);
    }
  }
}

static int conf_filter(
  const struct dirent *entry)
{
  char *dot = strrchr(entry->d_name, '.');
  if (dot != NULL && strcmp(dot, ".conf") == 0) {
    return  1;
  } else {
    return 0;
  }
}

static void parse_config_dir_at(
  struct moberg *config,
  int dirfd)
{
  if (dirfd >= 0) {
    struct dirent **entry = NULL;
    int n = scandirat(dirfd, ".", &entry, conf_filter, alphasort);
    for (int i = 0 ; i < n ; i++) {
      parse_config_at(config, dirfd, entry[i]->d_name);
      free(entry[i]);
    }
    free(entry);
  }
  
}

static int install_channel(struct moberg *moberg,
                           int index,
                           struct moberg_device* device,
                           struct moberg_channel *channel)
{
  if (channel) {
    struct moberg_channel *old = NULL;
    switch (channel->kind) {
      case chan_ANALOGIN:
        channel_list_get(&moberg->analog_in, index, &old);
        break;
      case chan_ANALOGOUT:
        channel_list_get(&moberg->analog_out, index, &old);
        break;
      case chan_DIGITALIN:
        channel_list_get(&moberg->digital_in, index, &old);
        break;
      case chan_DIGITALOUT:
        channel_list_get(&moberg->digital_out, index, &old);
        break;
      case chan_ENCODERIN:
        channel_list_get(&moberg->encoder_in, index, &old);
        break;
    }
    if (old) {
      old->down(old);
    }
    channel->up(channel);
    /* TODO: Clean up old channel */
    switch (channel->kind) {
      case chan_ANALOGIN:
        if (! channel_list_set(&moberg->analog_in, index, channel)) {
          goto err;
        }
        break;
      case chan_ANALOGOUT:
        if (! channel_list_set(&moberg->analog_out, index, channel)) {
          goto err;
        }
        break;
      case chan_DIGITALIN:
        if (! channel_list_set(&moberg->digital_in, index, channel)) {
          goto err;
        }
        break;
      case chan_DIGITALOUT:
        if (! channel_list_set(&moberg->digital_out, index, channel)) {
          goto err;
        }
        break;
      case chan_ENCODERIN:
        if (! channel_list_set(&moberg->encoder_in, index, channel)) {
          goto err;
        }
        break;
    }
  }
  return 1;
err:
  return 0;
}

static int install_config(struct moberg *moberg)
{
  struct moberg_channel_install install = {
    .context=moberg,
    .channel=install_channel
  };
  return moberg_config_install_channels(moberg->config, &install);
  /* TODO cleanup unused devices...*/
}

struct moberg *moberg_new(
  struct moberg_config *config)
{
  struct moberg *result = malloc(sizeof(*result));
  if (! result) {
    fprintf(stderr, "Failed to allocate moberg struct\n");
    goto err;
  }
  result->analog_in.capacity = 0;
  result->analog_in.value = NULL;
  result->analog_out.capacity = 0;
  result->analog_out.value = NULL;
  result->digital_in.capacity = 0;
  result->digital_in.value = NULL;
  result->digital_out.capacity = 0;
  result->digital_out.value = NULL;
  result->encoder_in.capacity = 0;
  result->encoder_in.value = NULL;
  result->deferred_action = NULL;
  if (config) {
    result->config = config;
  } else {
    result->config = NULL;
    
    /* Parse default configuration(s) */
    const char * const *config_paths = xdgSearchableConfigDirectories(NULL);
    const char * const *path;
    for (path = config_paths ; *path ; path++) {
      int dirfd1 = open(*path, O_DIRECTORY);
      if (dirfd >= 0) {
        parse_config_at(result, dirfd1, "moberg.conf");
        int dirfd2 = openat(dirfd1, "moberg.d", O_DIRECTORY);
        if (dirfd2 >= 0) { 
          parse_config_dir_at(result, dirfd2);
          close(dirfd2);
        }
        close(dirfd1);
      }
      free((char*)*path);
    }
    free((const char **)config_paths);
    
    /* Read environment default */
    /* Parse environment overrides */
  }
  install_config(result);
  run_deferred_actions(result);
  
err:
  return result;
}

void moberg_free(struct moberg *moberg)
{
  if (moberg) {
    moberg_config_free(moberg->config);
    channel_list_free(&moberg->analog_in);
    channel_list_free(&moberg->analog_out);
    channel_list_free(&moberg->digital_in);
    channel_list_free(&moberg->digital_out);
    channel_list_free(&moberg->encoder_in);
    run_deferred_actions(moberg);
    free(moberg);
  }
}

/* Input/output */

int moberg_analog_in_open(struct moberg *moberg,
                          int index,
                          struct moberg_analog_in *analog_in)
{
  struct moberg_channel *channel = NULL;
  channel_list_get(&moberg->analog_in, index, &channel);
  if (channel) {
    channel->open(channel);
    *analog_in = channel->action.analog_in;
    return 1;
  }
  return 0;
}

int moberg_analog_in_close(struct moberg *moberg,
                           int index,
                           struct moberg_analog_in analog_in)
{
  struct moberg_channel *channel = NULL;
  channel_list_get(&moberg->analog_in, index, &channel);
  if (channel && channel->action.analog_in.context == analog_in.context) {
    channel->close(channel);
  }
  return 1;
}



/* System init functionality (systemd/init/...) */

int moberg_start(
  struct moberg *moberg,
  FILE *f)
{
  return moberg_config_start(moberg->config, f);
}

int moberg_stop(
  struct moberg *moberg,
  FILE *f)
{
  return moberg_config_stop(moberg->config, f);
}

/* Intended for final cleanup actions (dlclose so far...) */

void moberg_deferred_action(struct moberg *moberg,
                            int (*action)(void *param),
                            void *param)
{
  struct deferred_action *deferred = malloc(sizeof(*deferred));
  if (deferred) {
    deferred->next = moberg->deferred_action;
    deferred->action = action;
    deferred->param = param;
    moberg->deferred_action = deferred;
  }
}
