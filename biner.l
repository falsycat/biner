%option noinput nounput noyywrap

%{
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>

#include "./tree.h"
#include "./zone.h"

#include "generated/biner.y.h"

static inline biner_zone_ptr(char) strnew_(void);
static inline bool parse_int_(intmax_t* v, int base);
static inline void count_(void);
%}

D [0-9]
I [A-Za-z_]
H [0-9A-Fa-f]

%x IN_COMMENT

%%

[ \t\r\n]+ count_();
"//".*     count_();

<INITIAL>"/*"        { count_(); BEGIN(IN_COMMENT); }
<IN_COMMENT>"*/"     { count_(); BEGIN(INITIAL); }
<IN_COMMENT>[^\*\n]+ count_();
<IN_COMMENT>"*"      count_();
<IN_COMMENT>"\n"     count_();

"==" { count_(); return EQUAL; }
"!=" { count_(); return NEQUAL; }
"<=" { count_(); return LESS_EQUAL; }
">=" { count_(); return GREATER_EQUAL; }
"&&" { count_(); return AND; }
"||" { count_(); return OR; }
"<<" { count_(); return BIT_LSHIFT; }
">>" { count_(); return BIT_RSHIFT; }

"const"  { count_(); return CONST; }
"enum"   { count_(); return ENUM; }
"struct" { count_(); return STRUCT; }
"union"  { count_(); return UNION; }

{I}({I}|{D})* { yylval.ptr = strnew_(); count_(); return IDENT; }

{D}+      { return parse_int_(&yylval.i, 10)? INTEGER: OVERFLOWN_INTEGER; }
0[xX]{H}+ { return parse_int_(&yylval.i, 16)? INTEGER: OVERFLOWN_INTEGER; }

. { count_(); return yytext[0]; }

%%
static inline uintptr_t strnew_(void) {
  return biner_zone_strnew(&biner_tree_parse_context_.zone, yytext);
}
static inline bool parse_int_(intmax_t* v, int base) {
  assert(v != NULL);

  char* end = NULL;
  *v = strtoimax(yytext, &end, base);
  return
    ((*v != INTMAX_MIN && *v != INTMAX_MAX) || errno != ERANGE) &&
    (INT64_MIN <= *v && *v <= INT64_MAX) &&
    (end != NULL && *end == 0);
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
