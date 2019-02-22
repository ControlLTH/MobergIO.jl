#ifndef __MOBERG_PARSER_H__
#define __MOBERG_PARSER_H__

#include <moberg_config.h>

struct moberg_parser_context;

struct moberg_config *moberg_config_parse(const char *buf);

#endif
