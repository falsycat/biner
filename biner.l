%option noinput nounput noyywrap

%{
#include <assert.h>
#include <errno.h>
#include <inttypes.h>

#include "./tree.h"
#include "./zone.h"

#include "generated/biner.y.h"

static inline biner_zone_ptr(char) strnew_(void);
static inline intmax_t parse_int_(int base);
static inline void count_(void);
%}

D [0-9]
I [A-Za-z_]
H [0-9A-Fa-f]

%%

"//".* count_();
[/][*][^*]*[*]+([^*/][^*]*[*]+)*[/] count_(); /* TODO: detect unterminated comment */

"enum"   { count_(); return ENUM; }
"struct" { count_(); return STRUCT; }

{I}({I}|{D})* { yylval.ptr = strnew_(); count_(); return IDENT; }

{D}+      { yylval.i = parse_int_(10); count_(); return INTEGER; }
0[xX]{H}+ { yylval.i = parse_int_(16); count_(); return INTEGER; }

[\+\-\*\/\=\.\(\)[\]\{\}\;\,] { count_(); return yytext[0]; }

(.|\n) count_();

%%
static inline uintptr_t strnew_(void) {
  return biner_zone_strnew(&biner_tree_parse_context_.zone, yytext);
}
static inline intmax_t parse_int_(int base) {
  char*          end = NULL;
  const intmax_t v   = strtoimax(yytext, &end, base);

  /* TODO: replace asserts with throwing error */
  assert((v != INTMAX_MIN && v != INTMAX_MAX) || errno != ERANGE);
  assert(INT64_MIN <= v && v <= INT64_MAX);
  assert(end != NULL && *end == 0);

  return v;
}
static inline void count_(void) {
  const char* s = yytext;
  while (*s) {
    if (*s == '\n') {
      biner_tree_parse_context_.column = 0;
      ++biner_tree_parse_context_.line;
    } else {
      ++biner_tree_parse_context_.column;
    }
    ++s;
  }
}
