/*
    moberg_module.h -- moberg module interface to parser

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
    
#ifndef __MOBERG_MODULE_H__
#define __MOBERG_MODULE_H__

#include <stdio.h>
#include <moberg_parser.h>

struct moberg_parser_token;

enum moberg_parser_token_kind {
  tok_none,
  tok_EOF,
  tok_LPAREN,
  tok_RPAREN,
  tok_LBRACE,
  tok_RBRACE,
  tok_LBRACKET,
  tok_RBRACKET,
  tok_EQUAL,
  tok_COMMA,
  tok_COLON,
  tok_SEMICOLON,
  tok_INTEGER,
  tok_IDENT,
  tok_STRING,
};

struct moberg_parser_ident {
  int length;
  const char *value;
};

struct moberg_parser_integer {
  int value;
};

struct moberg_parser_token {
  enum moberg_parser_token_kind kind;
  union {
    struct moberg_parser_ident idstr;
    struct moberg_parser_integer integer;
  } u;
};

int moberg_parser_acceptsym(
  struct moberg_parser_context *c,
  enum moberg_parser_token_kind kind,
  struct moberg_parser_token *token);

int moberg_parser_acceptkeyword(
  struct moberg_parser_context *c,
  const char *keyword);

struct moberg_status moberg_parser_failed(
  struct moberg_parser_context *c,
  FILE *f);

void moberg_deferred_action(
  struct moberg *moberg,
  int (*action)(void *param),
  void *param);

#endif
