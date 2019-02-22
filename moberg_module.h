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
    struct moberg_parser_ident ident;
    struct moberg_parser_ident string;
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

void moberg_parser_failed(
  struct moberg_parser_context *c,
  FILE *f);

#endif
