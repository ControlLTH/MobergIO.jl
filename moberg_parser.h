/*
    moberg_parser.h -- moberg parser interface

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
    
#ifndef __MOBERG_PARSER_H__
#define __MOBERG_PARSER_H__

#include <moberg.h>
#include <moberg_config.h>

struct moberg_parser_context;

struct moberg_config *moberg_parse(struct moberg* moberg,
                                   const char *buf);

#endif
