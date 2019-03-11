/*
    moberg_parser.c -- moberg parser interface

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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <moberg.h>
#include <moberg_inline.h>
#include <moberg_config.h>
#include <moberg_parser.h>
#include <moberg_module.h>
#include <moberg_device.h>

#define MAX_EXPECTED 10

typedef struct moberg_parser_context {
  struct moberg_config *config;
  const char *buf; /* Pointer to data to be parsed */
  const char *p;   /* current parse location */
  token_t token;
  struct {
    int n;
    const char *what[MAX_EXPECTED];
  } expected;
} context_t;


static const void nextsym_ident(context_t *c)
{
  c->token.kind = tok_IDENT;
  c->token.u.idstr.length = 1;
  c->token.u.idstr.value = c->p;
  c->p++;
  while (*c->p) {
    switch (*c->p) {
      case 'a'...'z':
      case 'A'...'Z':
      case '0'...'9':
      case '_':
        c->p++;
        c->token.u.idstr.length++;
        break;
      default:
        return;
    }
  }
}

static const void nextsym_integer(context_t *c)
{
  c->token.kind = tok_INTEGER;
  c->token.u.integer.value = 0;
  while (*c->p && '0' <= *c->p && *c->p <= '9') {
    c->token.u.integer.value *= 10;
    c->token.u.integer.value += *c->p - '0';
    c->p++;
  }
}

static const void nextsym_string(context_t *c)
{
  c->token.kind = tok_STRING;
  c->p++;
  c->token.u.idstr.value = c->p;
  c->token.u.idstr.length = 0;
  while (*c->p && *c->p != '"') {
    if (*c->p == '\\') {
      c->token.u.idstr.length++;
      c->p++;
    }
    if (*c->p) {
      c->token.u.idstr.length++;
      c->p++;
    }
  }
  c->p++;
}

static int nextsym(context_t *c)
{
  c->token.kind = tok_none;
  while (c->p && *c->p && c->token.kind == tok_none) {
    if (c->p[0] == '/' && c->p[1] == '*') {
      /* Skip comment */
      c->p += 2;
      while (*c->p && (c->p[0] != '*' || c->p[1] != '/')) {
        c->p++;
      }
      c->p += 2;
      continue;
    }
    switch (*c->p) {
      case ' ':
      case '\t':
      case '\n':
      case '\r':
        /* Skip whitespace */
        c->p++;
        break;
      case '(':
        c->token.kind = tok_LPAREN;
        c->p++;
        break;
      case ')':
        c->token.kind = tok_RPAREN;
        c->p++;
        break;
      case '{':
        c->token.kind = tok_LBRACE;
        c->p++;
        break;
      case '}':
        c->token.kind = tok_RBRACE;
        c->p++;
        break;
      case '[':
        c->token.kind = tok_LBRACKET;
        c->p++;
        break;
      case ']':
        c->token.kind = tok_RBRACKET;
        c->p++;
        break;
      case '=':
        c->token.kind = tok_EQUAL;
        c->p++;
        break;
      case ',':
        c->token.kind = tok_COMMA;
        c->p++;
        break;
      case ':':
        c->token.kind = tok_COLON;
        c->p++;
        break;
      case ';':
        c->token.kind = tok_SEMICOLON;
        c->p++;
        break;
      case '"':
        nextsym_string(c);
        break;
      case 'a'...'z':
      case 'A'...'Z':
      case '_':
        nextsym_ident(c);
        break;
      case '0'...'9':
        nextsym_integer(c);
        break;
      default:
        printf("UNEXPECTED %c\n\n", *c->p);
        c->p++;
        break;
    }
  }
  if (c->token.kind != tok_none) {
    return 1;
  } else {
    c->token.kind = tok_EOF;
    return 0;
  }
}

static int peeksym(context_t *c,
		   kind_t kind,
		   token_t *token)
{
  if (c->token.kind == kind) {
    if (token) {
      *token = c->token;
    }
    return 1;
  }
  return 0;
}

int moberg_parser_acceptsym(context_t *c,
                                   kind_t kind,
                                   token_t *token)
{
  if (c->token.kind == kind) {
    if (token) {
      *token = c->token;
    }
    nextsym(c);
    c->expected.n = 0;
    return 1;
  }
  if (c->expected.n < MAX_EXPECTED) {
    const char *what = NULL;
    switch (kind) {
    case tok_none: break;
      case tok_EOF: what = "<EOF>"; break;
    case tok_LPAREN: what = "("; break;
    case tok_RPAREN: what = ")"; break;
    case tok_LBRACE: what = "{"; break;
    case tok_RBRACE: what = "}"; break;
    case tok_LBRACKET: what = "["; break;
    case tok_RBRACKET: what = "]"; break;
    case tok_EQUAL: what = "="; break;
    case tok_COMMA: what = ","; break;
    case tok_COLON: what = ":"; break;
    case tok_SEMICOLON: what = ";"; break;
    case tok_INTEGER: what = "<INTEGER>"; break;
    case tok_IDENT: what = "<IDENT>"; break;
    case tok_STRING: what = "<STRING>"; break;
    }
    if (what) {
      c->expected.what[c->expected.n] = what;
      c->expected.n++;
    }
  }
  return 0;
}

int moberg_parser_acceptkeyword(context_t *c,
				       const char *keyword)
{
  token_t t;
  if (peeksym(c, tok_IDENT, &t) &&
      strncmp(keyword, t.u.idstr.value, t.u.idstr.length) == 0) {
    nextsym(c);
    c->expected.n = 0;
    return 1;
  }
  if (c->expected.n < MAX_EXPECTED) {
    c->expected.what[c->expected.n] = keyword;
    c->expected.n++;
  }
  return 0;
}

struct moberg_status moberg_parser_failed(
  struct moberg_parser_context *c,
  FILE *f)
{
  fprintf(f, "EXPECTED ");
  for (int i = 0 ; i < c->expected.n; i++) {
    if (i > 0) {
      fprintf(f, " | ");
    }
    fprintf(f, "'%s'", c->expected.what[i]);
  }
  fprintf(f, "\nGOT: ");
  switch (c->token.kind) {
    case tok_none: break;
    case tok_EOF: fprintf(f, "<EOF>"); break;
    case tok_LPAREN: fprintf(f, "("); break;
    case tok_RPAREN: fprintf(f, ")"); break;
    case tok_LBRACE: fprintf(f, "{"); break;
    case tok_RBRACE: fprintf(f, "}"); break;
    case tok_LBRACKET: fprintf(f, "["); break;
    case tok_RBRACKET: fprintf(f, "]"); break;
    case tok_EQUAL: fprintf(f, "="); break;
    case tok_COMMA: fprintf(f, ","); break;
    case tok_COLON: fprintf(f, ":"); break;
    case tok_SEMICOLON: fprintf(f, ";"); break;
    case tok_INTEGER:
      fprintf(f, "%d (<INTEGER>)", c->token.u.integer.value);
      break;
    case tok_IDENT:
      fprintf(f, "%.*s (<IDENT>)",
              c->token.u.idstr.length, c->token.u.idstr.value);
      break;
    case tok_STRING:
      fprintf(f, "\"%.*s'\" (<STRING>)",
              c->token.u.idstr.length, c->token.u.idstr.value);
      break;
  }
  fprintf(f, "\n%s\n", c->p);
  return MOBERG_ERRNO(EINVAL);
}


static int parse_map_range(context_t *c,
                           int *min,
                           int *max)
{
  token_t tok_min, tok_max;
  if (! acceptsym(c, tok_LBRACKET, NULL)) { goto syntax_err; }
  if (! acceptsym(c, tok_INTEGER, &tok_min)) { goto syntax_err; }
  if (acceptsym(c, tok_COLON, NULL)) {
    if (! acceptsym(c, tok_INTEGER, &tok_max)) { goto syntax_err; }
  } else {
    tok_max = tok_min;
  }
  if (! acceptsym(c, tok_RBRACKET, NULL)) { goto syntax_err; }
  if (! min && max) {
    fprintf(stderr, "Min or max is NULL\n");
    goto err;
  }
  *min = tok_min.u.integer.value;
  *max = tok_max.u.integer.value;
  return 1;
syntax_err:
  moberg_parser_failed(c, stderr);
err:
  return 0;
}

static struct moberg_status parse_map(context_t *c,
                                      struct moberg_device *device)
{
  enum moberg_channel_kind kind;
  int min, max;
  
  if (acceptkeyword(c, "analog_in")) { kind = chan_ANALOGIN; }
  else if (acceptkeyword(c, "analog_out")) { kind = chan_ANALOGOUT; }
  else if (acceptkeyword(c, "digital_in")) { kind = chan_DIGITALIN; }
  else if (acceptkeyword(c, "digital_out")) { kind = chan_DIGITALOUT; }
  else if (acceptkeyword(c, "encoder_in")) { kind = chan_ENCODERIN; }
  else { goto syntax_err; }
  if (! parse_map_range(c, &min, &max)) { goto syntax_err; }
  if (! acceptsym(c, tok_EQUAL, NULL)) { goto syntax_err; }
  struct moberg_status result = moberg_device_parse_map(device, c,
                                                        kind, min, max);
  if (! OK(result)) { return result; }
  if (! acceptsym(c, tok_SEMICOLON, NULL)) { goto syntax_err; }
  return MOBERG_OK;
syntax_err:
  return moberg_parser_failed(c, stderr);
}

static struct moberg_status parse_device(context_t *c,
                                         struct moberg_device *device)
{
  if (! acceptsym(c, tok_LBRACE, NULL)) { goto syntax_err; }
  for (;;) {
    if (acceptkeyword(c, "config")) {
      struct moberg_status result = moberg_device_parse_config(device, c);
      if (! OK(result)) {
        return result;
      }
    } else if (acceptkeyword(c, "map")) {
      struct moberg_status result = parse_map(c, device);
      if (! OK(result)) {
        return result;
      }
    } else if (acceptsym(c, tok_RBRACE, NULL)) {
      break;
    } else {
      goto syntax_err;
    }
  }
  return MOBERG_OK;
syntax_err:
  return moberg_parser_failed(c, stderr);
}

static struct moberg_status parse(struct moberg *moberg,
                                  context_t *c)
{
  struct moberg_status result = MOBERG_OK;
  for (;;) {
    if (acceptsym(c, tok_EOF, NULL)) {
      break;
    } else {
      token_t t;
      struct moberg_device *device;
      
      if (! acceptkeyword(c, "driver")) { goto syntax_err; }
      if (! acceptsym(c, tok_LPAREN, NULL)) { goto syntax_err; }
      if (! acceptsym(c, tok_IDENT, &t)) { goto syntax_err; }
      if (! acceptsym(c, tok_RPAREN, NULL)) { goto syntax_err; }

      char *name = strndup(t.u.idstr.value, t.u.idstr.length);
      if (! name) {
        fprintf(stderr, "Failed to allocate driver name '%.*s'\n",
                t.u.idstr.length, t.u.idstr.value);
        result = MOBERG_ERRNO(ENOMEM);
        goto err_result;
      }
      device = moberg_device_new(moberg, name);
      free(name);
      if (! device) {
        result = MOBERG_ERRNO(ENOMEM);
        goto err_result;
      }
      result = parse_device(c, device);
      if (! OK(result)) {
        goto device_free;
      }
      result = moberg_config_add_device(c->config, device);
      if (! OK(result)) {
        goto device_free;
      }
      continue;
    device_free:
      moberg_device_free(device);      
      goto err_result;
    }
  }
  return MOBERG_OK;
err_result:
  return result;
syntax_err:  
  return moberg_parser_failed(c, stderr);
}

struct moberg_config *moberg_parse(struct moberg *moberg,
                                   const char *buf)
{
  context_t context;

  context.config = moberg_config_new();
  if (context.config) {
    context.expected.n = 0;
    context.buf = buf;
    context.p = context.buf;
    nextsym(&context);
    if (! OK(parse(moberg, &context))) {
      moberg_config_free(context.config);
      context.config = NULL;
    }
  }

  return context.config;
}
  

