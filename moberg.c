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

struct moberg_digital_in_t {
  int index;
  char *driver;
};

struct moberg {
  struct moberg_config *config;
  struct {
    int count;
    struct moberg_digital_in_t *value;
  } digital_in;
};

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
          printf("Parsing... %s %d %d\n", pathname, dirfd, fd);
          struct moberg_config *config = moberg_parse(buf);
          printf("-> %p\n", config);
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

static int install_config(struct moberg *moberg)
{
  struct moberg_install_channels install = {
    .context=moberg
  };
  return moberg_config_install_channels(moberg->config, &install);
}

struct moberg *moberg_new(
  struct moberg_config *config)
{
  struct moberg *result = malloc(sizeof(*result));
  if (! result) {
    fprintf(stderr, "Failed to allocate moberg struct\n");
    goto err;
  }
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
  
err:
  return result;
}

void moberg_free(struct moberg *moberg)
{
  if (moberg) {
    moberg_config_free(moberg->config);
    free(moberg);
  }
}

enum moberg_status moberg_start(
  struct moberg *moberg,
  FILE *f)
{
  return moberg_OK;
}


enum moberg_status moberg_stop(
  struct moberg *moberg,
  FILE *f)
{
  return moberg_OK;
}

