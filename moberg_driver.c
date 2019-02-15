#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <moberg_driver.h>
#include <dlfcn.h>

struct moberg_driver *moberg_open_driver(struct moberg_config_parser_ident id)
{
  struct moberg_driver *result = NULL;

  char *driver_name = malloc(sizeof("libmoberg_.so") + id.length + 1);
  if (!driver_name) { goto out; }
  sprintf(driver_name, "libmoberg_%.*s.so", id.length, id.value);
  printf("%s", driver_name);
  void *handle = dlopen(driver_name, RTLD_LAZY || RTLD_DEEPBIND);
  if (! handle) { goto free_driver_name; }
  struct moberg_driver_module *module =
    (struct moberg_driver_module *) dlsym(handle, "moberg_module");
  if (! module) { goto dlclose_driver; }
  result = malloc(sizeof(*result));
  if (! result) { goto dlclose_driver; }
  result->handle = handle;
  result->module = *module;
  goto free_driver_name;

dlclose_driver:
    dlclose(handle);
free_driver_name:
    free(driver_name);
out:
    return result;
}

void moberg_close_driver(struct moberg_driver *driver)
{
  dlclose(driver->handle);
  free(driver);
}

