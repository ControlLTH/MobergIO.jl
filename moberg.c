/*
    moberg.c --  interface to moberg I/O system

    Copyright (C) 2019 Anders Blomdell <anders.blomdell@gmail.com>

    This file is part of Moberg.

    Moberg is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
#include <errno.h>
#include <moberg.h>
#include <moberg_config.h>
#include <moberg_inline.h>
#include <moberg_module.h>
#include <moberg_parser.h>

struct moberg {
  int should_free;
  int open_channels;
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
    void *new = realloc(list->value, capacity * sizeof(*list->value));
    if (!new) {
      goto err;
    }
    void *p = new + list->capacity * sizeof(*list->value);
    memset(p, 0, (capacity - list->capacity) * sizeof(*list->value));
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

static struct moberg_status install_channel(
  struct moberg *moberg,
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
  return MOBERG_OK;
err:
  return MOBERG_ERRNO(ENOMEM);
}

static int install_config(struct moberg *moberg)
{
  struct moberg_channel_install install = {
    .context=moberg,
    .channel=install_channel
  };
  if (! moberg->config) {
    fprintf(stderr, "No moberg configuration found\n");
    return 0;
  } else {
    return moberg_config_install_channels(moberg->config, &install);
  }
}

int moberg_OK(struct moberg_status status)
{
  return status.result == 0;
}

struct moberg *moberg_new()
{
  struct moberg *result = malloc(sizeof(*result));
  if (! result) {
    fprintf(stderr, "Failed to allocate moberg struct\n");
    goto err;
  }
  memset(result, 0, sizeof(*result));

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
  
  /* TODO: Read & parse environment overrides */

  install_config(result);
  run_deferred_actions(result);
  
err:
  return result;
}

static void free_if_unused(struct moberg *moberg)
{
  if (moberg->should_free && moberg->open_channels == 0) {
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

void moberg_free(struct moberg *moberg)
{
  if (moberg) {
    moberg->should_free = 1;
    free_if_unused(moberg);
  }
}


/* Input/output */

struct moberg_status moberg_analog_in_open(
  struct moberg *moberg,
  int index,
  struct moberg_analog_in *analog_in)
{
  if (! analog_in) {
    return MOBERG_ERRNO(EINVAL);
  }
  struct moberg_channel *channel = NULL;
  channel_list_get(&moberg->analog_in, index, &channel);
  if (! channel) {
    return MOBERG_ERRNO(ENODEV);
  } 
  struct moberg_status result = channel->open(channel);
  if (! OK(result)) {
    return result;
  }
  moberg->open_channels++;
  *analog_in = channel->action.analog_in;
  return MOBERG_OK;
}

struct moberg_status moberg_analog_in_close(
  struct moberg *moberg,
  int index,
  struct moberg_analog_in analog_in)
{
  struct moberg_channel *channel = NULL;
  channel_list_get(&moberg->analog_in, index, &channel);
  if (! channel) {
    return MOBERG_ERRNO(ENODEV);
  }
  if (channel->action.analog_in.context != analog_in.context) {
    return MOBERG_ERRNO(EINVAL);
  }
  struct moberg_status result = channel->close(channel);
  moberg->open_channels--;
  free_if_unused(moberg);
  if (! OK(result)) {
    return result;
  }
  return MOBERG_OK;
}

struct moberg_status moberg_analog_out_open(
  struct moberg *moberg,
  int index,
  struct moberg_analog_out *analog_out)
{
  if (! analog_out) {
    return MOBERG_ERRNO(EINVAL);
  }
  struct moberg_channel *channel = NULL;
  channel_list_get(&moberg->analog_out, index, &channel);
  if (! channel) {
    return MOBERG_ERRNO(ENODEV);
  } 
  struct moberg_status result = channel->open(channel);
  if (! OK(result)) {
    return result;
  }
  moberg->open_channels++;
  *analog_out = channel->action.analog_out;
  return MOBERG_OK;
}

struct moberg_status moberg_analog_out_close(
  struct moberg *moberg,
  int index,
  struct moberg_analog_out analog_out)
{
  struct moberg_channel *channel = NULL;
  channel_list_get(&moberg->analog_out, index, &channel);
  if (! channel) {
    return MOBERG_ERRNO(ENODEV);
  }
  if (channel->action.analog_out.context != analog_out.context) {
    return MOBERG_ERRNO(EINVAL);
  }
  struct moberg_status result = channel->close(channel);
  moberg->open_channels--;
  free_if_unused(moberg);
  if (! OK(result)) {
    return result;
  }
  return MOBERG_OK;
}

struct moberg_status moberg_digital_in_open(
  struct moberg *moberg,
  int index,
  struct moberg_digital_in *digital_in)
{
  if (! digital_in) {
    return MOBERG_ERRNO(EINVAL);
  }
  struct moberg_channel *channel = NULL;
  channel_list_get(&moberg->digital_in, index, &channel);
  if (! channel) {
    return MOBERG_ERRNO(ENODEV);
  } 
  struct moberg_status result = channel->open(channel);
  if (! OK(result)) {
    return result;
  }
  moberg->open_channels++;
  *digital_in = channel->action.digital_in;
  return MOBERG_OK;
}

struct moberg_status moberg_digital_in_close(
  struct moberg *moberg,
  int index,
  struct moberg_digital_in digital_in)
{
  struct moberg_channel *channel = NULL;
  channel_list_get(&moberg->digital_in, index, &channel);
  if (! channel) {
    return MOBERG_ERRNO(ENODEV);
  }
  if (channel->action.digital_in.context != digital_in.context) {
    return MOBERG_ERRNO(EINVAL);
  }
  struct moberg_status result = channel->close(channel);
  moberg->open_channels--;
  free_if_unused(moberg);
  if (! OK(result)) {
    return result;
  }
  return MOBERG_OK;
}

struct moberg_status moberg_digital_out_open(
  struct moberg *moberg,
  int index,
  struct moberg_digital_out *digital_out)
{
  if (! digital_out) {
    return MOBERG_ERRNO(EINVAL);
  }
  struct moberg_channel *channel = NULL;
  channel_list_get(&moberg->digital_out, index, &channel);
  if (! channel) {
    return MOBERG_ERRNO(ENODEV);
  } 
  struct moberg_status result = channel->open(channel);
  if (! OK(result)) {
    return result;
  }
  moberg->open_channels++;
  *digital_out = channel->action.digital_out;
  return MOBERG_OK;
}

struct moberg_status moberg_digital_out_close(
  struct moberg *moberg,
  int index,
  struct moberg_digital_out digital_out)
{
  struct moberg_channel *channel = NULL;
  channel_list_get(&moberg->digital_out, index, &channel);
  if (! channel) {
    return MOBERG_ERRNO(ENODEV);
  }
  if (channel->action.digital_out.context != digital_out.context) {
    return MOBERG_ERRNO(EINVAL);
  }
  struct moberg_status result = channel->close(channel);
  moberg->open_channels--;
  free_if_unused(moberg);
  if (! OK(result)) {
    return result;
  }
  return MOBERG_OK;
}

struct moberg_status moberg_encoder_in_open(
  struct moberg *moberg,
  int index,
  struct moberg_encoder_in *encoder_in)
{
  if (! encoder_in) {
    return MOBERG_ERRNO(EINVAL);
  }
  struct moberg_channel *channel = NULL;
  channel_list_get(&moberg->encoder_in, index, &channel);
  if (! channel) {
    return MOBERG_ERRNO(ENODEV);
  } 
  struct moberg_status result = channel->open(channel);
  if (! OK(result)) {
    return result;
  }
  moberg->open_channels++;
  *encoder_in = channel->action.encoder_in;
  return MOBERG_OK;
}

struct moberg_status moberg_encoder_in_close(
  struct moberg *moberg,
  int index,
  struct moberg_encoder_in encoder_in)
{
  struct moberg_channel *channel = NULL;
  channel_list_get(&moberg->encoder_in, index, &channel);
  if (! channel) {
    return MOBERG_ERRNO(ENODEV);
  }
  if (channel->action.encoder_in.context != encoder_in.context) {
    return MOBERG_ERRNO(EINVAL);
  }
  struct moberg_status result = channel->close(channel);
  moberg->open_channels--;
  free_if_unused(moberg);
  if (! OK(result)) {
    return result;
  }
  return MOBERG_OK;
}

/* System init functionality (systemd/init/...) */

struct moberg_status moberg_start(
  struct moberg *moberg,
  FILE *f)
{
  return moberg_config_start(moberg->config, f);
}

struct moberg_status moberg_stop(
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
