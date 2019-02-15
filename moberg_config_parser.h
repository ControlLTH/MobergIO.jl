#ifndef __MOBERG_CONFIG_PARSER_H__
#define __MOBERG_CONFIG_PARSER_H__

struct moberg_config_parser_context;
struct moberg_config_parser_token;

enum moberg_config_parser_token_kind {
  tok_none,
  tok_LBRACE = '{',
  tok_RBRACE,
  tok_LBRACKET,
  tok_RBRACKET,
  tok_EQUAL,
  tok_COLON,
  tok_SEMICOLON,
  tok_INTEGER,
  tok_CONFIG,
  tok_MAP,
  tok_ANALOGIN,
  tok_ANALOGOUT,
  tok_DIGITALIN,
  tok_DIGITALOUT,
  tok_ENCODERIN,
  tok_IDENT,
};

struct moberg_config_parser_token {
  enum moberg_config_parser_token_kind kind;
  union {
    struct moberg_config_parser_ident {
      int length;
      const char *value;
    } ident;
    struct moberg_config_parser_integer {
      int value;
    } integer;
  } u;
};

int moberg_config_parser_acceptsym(
  struct moberg_config_parser_context *c,
  enum moberg_config_parser_token_kind kind,
  struct moberg_config_parser_token *token);

#endif
