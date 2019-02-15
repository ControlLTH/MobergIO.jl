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
#include "moberg.h"

struct moberg_digital_in_t {
  int index;
  char *driver;
};

struct moberg_t {
  struct {
    int count;
    struct moberg_digital_in_t *value;
  } digital_in;
};

static void parse_config_at(struct moberg_t *config,
                            int dirfd,
                            const char *pathname)
{
  if (dirfd >= 0) {
    int fd = openat(dirfd, pathname, O_RDONLY);
    if (fd >= 0) {
      printf("Parsing... %s %d %d\n", pathname, dirfd, fd);
      close(fd);
    }
  }
  
}

static int conf_filter(const struct dirent *entry)
{
  char *dot = strrchr(entry->d_name, '.');
  if (dot != NULL && strcmp(dot, ".conf") == 0) {
    return  1;
  } else {
    return 0;
  }
}

static void parse_config_dir_at(struct moberg_t *config,
                                int dirfd)
{
  if (dirfd >= 0) {
    struct dirent **entry = NULL;
    int n = scandirat(dirfd, ".", &entry, conf_filter, alphasort);
    for (int i = 0 ; i < n ; i++) {
      parse_config_at(config, dirfd, entry[i]->d_name);
    }
    free(entry);
  }
  
}

const struct moberg_t *moberg_init()
{
  struct moberg_t *result = malloc(sizeof(struct moberg_t));
  
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
  }
  free((const char **)config_paths);

  /* Read local default */
  /* Read environment default */
  /* Parse environment overrides */
  
  
  return result;
}
