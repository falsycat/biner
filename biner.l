%option noinput nounput noyywrap

%{

#include <assert.h>
#include <errno.h>
#include <inttypes.h>

#include "./tree.h"
#include "./zone.h"

#include "generated/biner.y.h"

#define ctx (biner_tree_parse_context_)

static inline uintptr_t strnew_(const char* str) {
  return biner_zone_strnew(&ctx.zone, str);
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
%}

D [0-9]
I [A-Za-z_]
H [0-9A-Fa-f]

%%

"//".* ;
[/][*][^*]*[*]+([^*/][^*]*[*]+)*[/] ; /* TODO: detect unterminated comment */

"struct" return STRUCT;

{I}({I}|{D})* { yylval.ptr = strnew_(yytext); return IDENT; }

{D}+      { yylval.i = parse_int_(10); return INTEGER; }
0[xX]{H}+ { yylval.i = parse_int_(16); return INTEGER; }

[\+\-\*\/\.\(\)[\]\{\}\;] return yytext[0];

(.|\n) ;

%%
