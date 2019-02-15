#ifndef __MOBERG_H__
#define __MOBERG_H__

struct moberg_t;
struct moberg_digital_in_t;


const struct moberg_t *moberg_init();

struct moberg_digital_in_t *moberg_open_digital_in(
  const struct moberg_t *handle,
  int channel);
  

#endif
