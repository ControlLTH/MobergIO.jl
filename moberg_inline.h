/*
    moberg_inline.h -- useful short names for moberg implementation

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
    
#ifndef __MOBERG_INLINE_H__
#define __MOBERG_INLINE_H__

#include <moberg.h>
#include <moberg_module.h>

/* Error handling */

#define MOBERG_OK (struct moberg_status){ .result=0 }
#define MOBERG_ERRNO(errno) (struct moberg_status){ .result=errno }

static int inline OK(struct moberg_status status)
{
  return moberg_OK(status);
}

/* Config file parsing */

typedef enum moberg_parser_token_kind kind_t;
typedef struct moberg_parser_token token_t;
typedef struct moberg_parser_ident ident_t;
typedef struct moberg_parser_context context_t;

static inline int acceptsym(context_t *c,
                            kind_t kind,
                            token_t *token)
{
  return moberg_parser_acceptsym(c, kind, token);
}
  
static inline int acceptkeyword(context_t *c,
				const char *keyword)
{
  return moberg_parser_acceptkeyword(c, keyword);
}

#endif


