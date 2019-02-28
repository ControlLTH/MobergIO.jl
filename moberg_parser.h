#ifndef __MOBERG_PARSER_H__
#define __MOBERG_PARSER_H__

#include <moberg.h>
#include <moberg_config.h>

struct moberg_parser_context;

struct moberg_config *moberg_parse(struct moberg* moberg,
                                   const char *buf);

#endif
