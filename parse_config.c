#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <moberg_config_parser.h>
#include <moberg_driver.h>

typedef enum moberg_config_parser_token_kind kind_t;
typedef struct moberg_config_parser_token token_t;
typedef struct moberg_config_parser_ident ident_t;
typedef struct moberg_config_parser_context context_t;

static inline int acceptsym(context_t *c,
			   kind_t kind,
			   token_t *token)
{
  return moberg_config_parser_acceptsym(c, kind, token);
}
  
static inline int acceptkeyword(context_t *c,
				const char *keyword)
{
  return moberg_config_parser_acceptkeyword(c, keyword);
}
 
#define MAX_EXPECTED 10
static char expected_char[256][2];

typedef struct moberg_config_parser_context {
  char *buf;      /* Pointer to data to be parsed */
  const char *p;  /* current parse location */
  token_t token;
  struct {
    int n;
    const char *what[MAX_EXPECTED];
  } expected;
} context_t;

static const void nextsym_ident(context_t *c)
{
  c->token.u.ident.length = 1;
  c->token.u.ident.value = c->p;
  c->p++;
  while (*c->p) {
    switch (*c->p) {
      case 'a'...'z':
      case 'A'...'Z':
      case '0'...'9':
      case '_':
        c->p++;
        c->token.u.ident.length++;
        break;
      default:
        goto out;
        break;
    }
  }
out: ;
  const char *v = c->token.u.ident.value;
  int l = c->token.u.ident.length;
  c->token.kind = tok_IDENT;
  printf("IDENT: %.*s %d\n", l, v, c->token.kind);
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
  printf("INTEGER: %d\n", c->token.u.integer.value);
}

static const void nextsym_string(context_t *c)
{
  printf("STTRING");
  c->token.kind = tok_STRING;
  c->p++;
  c->token.u.string.value = c->p;
  c->token.u.string.length = 0;
  while (*c->p && *c->p != '"') {
    if (*c->p == '\\') {
      c->token.u.string.length++;
      c->p++;
    }
    if (*c->p) {
      c->token.u.string.length++;
      c->p++;
    }
  }
  c->p++;
  printf("STRING: %.*s\n", c->token.u.string.length, c->token.u.string.value);
}

static int nextsym(context_t *c)
{
  c->token.kind = tok_none;
  while (c->p && *c->p && c->token.kind == tok_none) {
    if (c->p[0] == '/' && c->p[1] == '*') {
      printf("Skipping COMMENT\n");
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
  printf("TOKEN %d %c\n\n", c->token.kind, c->token.kind<255?c->token.kind:' ');
  if (c->token.kind != tok_none) {
    return 1;
  } else {
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

int moberg_config_parser_acceptsym(context_t *c,
                                   kind_t kind,
                                   token_t *token)
{
  if (c->token.kind == kind) {
    printf("ACCEPT %d %s", c->token.kind, expected_char[kind]);
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
    case tok_LBRACE: what = "{"; break;
    case tok_RBRACE: what = "}"; break;
    case tok_LBRACKET: what = "["; break;
    case tok_RBRACKET: what = "]"; break;
    case tok_EQUAL: what = "="; break;
    case tok_COLON: what = ":"; break;
    case tok_SEMICOLON: what = "y;"; break;
    case tok_INTEGER: what = "<INTEGER>"; break;
    case tok_IDENT: what = "<IDENT>"; break;
    case tok_STRING: what = "<STRING>"; break;
    }
    if (what) {
      c->expected.what[c->expected.n] = what;
      c->expected.n++;
    }
  }
  printf("REJECT %d (%d)", kind, c->token.kind);
  return 0;
}

int moberg_config_parser_acceptkeyword(context_t *c,
				       const char *keyword)
{
  token_t t;
  if (peeksym(c, tok_IDENT, &t) &&
      strncmp(keyword, t.u.ident.value, t.u.ident.length) == 0) {
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

void moberg_config_parser_failed(
  struct moberg_config_parser_context *c,
  FILE *f)
{
  fprintf(f, "EXPECTED ");
  for (int i = 0 ; i < c->expected.n ; i++) {
    fprintf(f, "%s ", c->expected.what[i]);
  }
  const char *what = "";
  switch (c->token.kind) {
  case tok_none: break;
  case tok_LBRACE: what = "{"; break;
  case tok_RBRACE: what = "}"; break;
  case tok_LBRACKET: what = "["; break;
  case tok_RBRACKET: what = "]"; break;
  case tok_EQUAL: what = "="; break;
  case tok_COLON: what = ":"; break;
  case tok_SEMICOLON: what = ";"; break;
  case tok_INTEGER: what = "<INTEGER>"; break;
  case tok_IDENT: what = "<IDENT>"; break;
  case tok_STRING: what = "<STRING>"; break;
  }
  
  fprintf(f, "\nGOT %s %s\n", what, c->p);
}


static int parse_map_range(context_t *c)
{
  token_t low, high;
  if (! acceptsym(c, tok_LBRACKET, NULL)) { goto err; }
  if (! acceptsym(c, tok_INTEGER, &low)) { goto err; }
  if (acceptsym(c, tok_COLON, NULL)) {
    if (! acceptsym(c, tok_INTEGER, &high)) { goto err; }
  } else {
    high = low;
  }
  if (! acceptsym(c, tok_RBRACKET, NULL)) { goto err; }
  return 1;
err:
  moberg_config_parser_failed(c, stderr);
  exit(1);
  return 0;
}

static int parse_map(context_t *c,
                     struct moberg_driver *driver)
{
  printf("parsemap");
  if (acceptkeyword(c, "analog_in") ||
      acceptkeyword(c, "analog_out") ||
      acceptkeyword(c, "digital_in") ||
      acceptkeyword(c, "digital_out") ||
      acceptkeyword(c, "encoder_in")) {
    if (! parse_map_range(c)) { goto err; }
    if (! acceptsym(c, tok_EQUAL, NULL)) { goto err; }
    driver->module.parse_map(c, 0);
    if (! parse_map_range(c)) { goto err; }
    if (! acceptsym(c, tok_SEMICOLON, NULL)) { goto err; }
  } else {
    goto err;
  }
  return 1;
err:    
  moberg_config_parser_failed(c, stderr);
  exit(1);
  return 0;
}

static int parse_device(context_t *c,
                        struct moberg_driver *driver)
{
  if (! acceptsym(c, tok_LBRACE, NULL)) { goto err; }
  for (;;) {
    if (acceptkeyword(c, "config")) {
      driver->module.parse_config(c);
    } else if (acceptkeyword(c, "map")) {
      parse_map(c, driver);
    } else if (acceptsym(c, tok_RBRACE, NULL)) {
      break;
    } else {
      goto err;
    }
  }
  return 1;
err:
  return 0;
}

static int parse_driver(context_t *c,
                        ident_t name)
{
  struct moberg_driver *driver = moberg_open_driver(name);
  if (! driver) {
    printf("Driver not found\n");
    goto err;
  } else {
    int OK = parse_device(c, driver);
    moberg_close_driver(driver);
    if (! OK) { goto err; } 
  }
  return 1;
err:
  return 0;
}

static int parse_config(context_t *c)
{
  for (;;) {
    token_t t;
    if (acceptsym(c, tok_IDENT, &t)) {
      printf("DRIVER=%.*s\n", t.u.ident.length, t.u.ident.value);
      if (! parse_driver(c, t.u.ident)) { goto err; }
    }
  }
  return 1;
err:
  printf("Failed!!");
  exit (1);
  return 0;
}

static char *read_fd(int fd)
{
  char *result = malloc(10000); /* HACK */
  int pos = read(fd, result, 10000);
  result[pos] = 0;
  return result;
}

int main(int argc, char *argv[])
{
  context_t context;
  int fd = open(argv[1], O_RDONLY);
  context.buf = read_fd(fd);
  context.p = context.buf;
  close(fd);
  printf("%s\n", context.buf);
  nextsym(&context);
  parse_config(&context);
  free(context.buf);
}
