%{
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./tree.h"
#include "./zone.h"

#define ctx (biner_tree_parse_context_)

#define alloc_(T) (biner_zone_alloc(&ctx.zone, sizeof(T)))
#define ref(T, p) ((T*) (ctx.zone.ptr+p))

extern char* yytext;

extern int yylex(void);

#define yyerrorf(fmt, ...)  \
  fprintf(stderr, "[%zu:%zu] "fmt"\n", ctx.line+1, ctx.column+1, __VA_ARGS__)
#define yyerror(msg)  \
  fprintf(stderr, "[%zu:%zu] "msg"\n", ctx.line+1, ctx.column+1)

static inline biner_zone_ptr(biner_tree_decl_t) find_decl_by_name_(
  biner_zone_ptr(char) name);

static inline biner_zone_ptr(biner_tree_struct_member_t)
find_struct_member_by_name_(
  biner_zone_ptr(biner_tree_struct_member_t) last_member,
  biner_zone_ptr(char)                       name);

static inline biner_zone_ptr(biner_tree_struct_member_t)
find_child_struct_member_by_name_(
    biner_zone_ptr(biner_tree_struct_member_t) member,
    biner_zone_ptr(char)                       name);

static inline biner_zone_ptr(biner_tree_struct_member_t)
unwrap_struct_member_ref_(
    biner_zone_ptr(biner_tree_struct_member_reference_t) memref);
%}

%union {
  int64_t   i;
  uintptr_t ptr;
}

%token STRUCT
%token <ptr> IDENT
%token <i>   INTEGER;

%type <ptr> decl_list decl
%type <ptr> struct_body struct_member struct_member_type struct_member_reference
%type <ptr> expr add_expr mul_expr operand

%start decl_list

%%

decl_list
  : decl {
    *ref(biner_tree_root_t, ctx.root) = (biner_tree_root_t) {
      .decls = $1,
    };
    $$ = ctx.root;
  }
  | decl_list decl {
    ref(biner_tree_decl_t, $2)->prev  = ref(biner_tree_root_t, $1)->decls;
    ref(biner_tree_root_t, $1)->decls = $2;
    $$ = $1;
  }
  ;

decl
  : STRUCT IDENT '{' struct_body '}' ';' {
    if (find_decl_by_name_($2) != 0) {
      yyerrorf("duplicated declaration of '%s'", ref(char, $2));
      YYABORT;
    }
    $$ = alloc_(biner_tree_decl_t);
    *ref(biner_tree_decl_t, $$) = (biner_tree_decl_t) {
      .name   = $2,
      .member = $4,
    };
    ctx.last_decl   = $$;
    ctx.last_member = 0;
  }
  ;

struct_body
  : struct_member {
    $$ = ctx.last_member = $1;
  }
  | struct_body struct_member {
    ref(biner_tree_struct_member_t, $2)->prev = $1;
    $$ = ctx.last_member = $2;
  }
  ;

struct_member
  : struct_member_type IDENT ';' {
    if (find_struct_member_by_name_(ctx.last_member, $2) != 0) {
      yyerrorf("duplicated struct member of '%s'", ref(char, $2));
      YYABORT;
    }
    $$ = alloc_(biner_tree_struct_member_t);
    *ref(biner_tree_struct_member_t, $$) =
      (biner_tree_struct_member_t) {
        .type = $1,
        .name = $2,
      };
  }
  ;

struct_member_type
  : IDENT {
    /* TODO: upgrade generic type to user-defined type. */
    $$ = alloc_(biner_tree_struct_member_type_t);
    *ref(biner_tree_struct_member_type_t, $$) =
      (biner_tree_struct_member_type_t) {
        .kind      = BINER_TREE_STRUCT_MEMBER_TYPE_KIND_GENERIC,
        .generic   = $1,
        .qualifier = BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_NONE,
      };
  }
  | IDENT '[' expr ']' {
    $$ = alloc_(biner_tree_struct_member_type_t);
    *ref(biner_tree_struct_member_type_t, $$) =
      (biner_tree_struct_member_type_t) {
        .kind      = BINER_TREE_STRUCT_MEMBER_TYPE_KIND_GENERIC,
        .generic   = $1,
        .qualifier = BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_DYNAMIC_ARRAY,
        .expr      = $3,
      };
  }
  ;

expr
  : add_expr { $$ = $1; }
  ;

add_expr
  : mul_expr
  | add_expr '+' mul_expr {
    $$ = alloc_(biner_tree_expr_t);
    *ref(biner_tree_expr_t, $$) = (biner_tree_expr_t) {
      .type     = BINER_TREE_EXPR_TYPE_OPERATOR_ADD,
      .operands = {$1, $3},
    };
  }
  | add_expr '-' mul_expr {
    $$ = alloc_(biner_tree_expr_t);
    *ref(biner_tree_expr_t, $$) = (biner_tree_expr_t) {
      .type     = BINER_TREE_EXPR_TYPE_OPERATOR_SUB,
      .operands = {$1, $3},
    };
  }
  ;

mul_expr
  : operand
  | mul_expr '*' operand {
    $$ = alloc_(biner_tree_expr_t);
    *ref(biner_tree_expr_t, $$) = (biner_tree_expr_t) {
      .type     = BINER_TREE_EXPR_TYPE_OPERATOR_MUL,
      .operands = {$1, $3},
    };
  }
  | mul_expr '/' operand {
    $$ = alloc_(biner_tree_expr_t);
    *ref(biner_tree_expr_t, $$) = (biner_tree_expr_t) {
      .type     = BINER_TREE_EXPR_TYPE_OPERATOR_DIV,
      .operands = {$1, $3},
    };
  }
  ;

operand
  : INTEGER {
    $$ = alloc_(biner_tree_expr_t);
    *ref(biner_tree_expr_t, $$) = (biner_tree_expr_t) {
      .type = BINER_TREE_EXPR_TYPE_OPERAND_INTEGER,
      .i    = $1,
    };
  }
  | struct_member_reference {
    $$ = alloc_(biner_tree_expr_t);
    *ref(biner_tree_expr_t, $$) = (biner_tree_expr_t) {
      .type = BINER_TREE_EXPR_TYPE_OPERAND_REFER,
      .r    = $1,
    };
  }
  | '(' expr ')' { $$ = $2; }
  ;

struct_member_reference
  : IDENT {
    const biner_zone_ptr(biner_tree_struct_member_t) member =
      find_struct_member_by_name_(ctx.last_member, $1);
    if (member == 0) {
      yyerrorf("unknown member '%s'", ref(char, $1));
      YYABORT;
    }

    $$ = alloc_(biner_tree_struct_member_reference_t);
    *ref(biner_tree_struct_member_reference_t, $$) =
      (biner_tree_struct_member_reference_t) { .member = member, };
  }
  | struct_member_reference '.' IDENT {
    const biner_zone_ptr(biner_tree_struct_member_t) member =
      find_child_struct_member_by_name_(unwrap_struct_member_ref_($1), $3);
    if (member == 0) {
      yyerrorf("unknown member '%s'", ref(char, $3));
      YYABORT;
    }

    $$ = alloc_(biner_tree_struct_member_reference_t);
    *ref(biner_tree_struct_member_reference_t, $$) =
      (biner_tree_struct_member_reference_t) { .member = member, };
  }
  ;

%%
static inline biner_zone_ptr(biner_tree_decl_t) find_decl_by_name_(
    biner_zone_ptr(char) name) {
  biner_zone_ptr(biner_tree_decl_t) itr = ctx.last_decl;
  while (itr) {
    const biner_tree_decl_t* decl = ref(biner_tree_decl_t, itr);
    if (strcmp(ref(char, decl->name), ref(char, name)) == 0) return itr;
    itr = decl->prev;
  }
  return 0;
}

static inline biner_zone_ptr(biner_tree_struct_member_t)
find_struct_member_by_name_(
    biner_zone_ptr(biner_tree_struct_member_t) last_member,
    biner_zone_ptr(char)                       name) {
  biner_zone_ptr(biner_tree_struct_member_t) itr = last_member;
  while (itr) {
    const biner_tree_struct_member_t* m = ref(biner_tree_struct_member_t, itr);
    if (strcmp(ref(char, m->name), ref(char, name)) == 0) return itr;
    itr = m->prev;
  }
  return 0;
}

static inline biner_zone_ptr(biner_tree_struct_member_t)
find_child_struct_member_by_name_(
    biner_zone_ptr(biner_tree_struct_member_t) member,
    biner_zone_ptr(char)                       name) {
  const biner_tree_struct_member_t* m = ref(biner_tree_struct_member_t, member);

  const biner_tree_struct_member_type_t* t =
    ref(biner_tree_struct_member_type_t, m->type);
  if (t->kind != BINER_TREE_STRUCT_MEMBER_TYPE_KIND_USER_DEFINED) {
    yyerrorf("typeof '%s' is not user-defined", ref(char, m->name));
    return 0;
  }
  const biner_tree_decl_t* d = ref(biner_tree_decl_t, t->decl);
  if (d->type != BINER_TREE_DECL_TYPE_STRUCT) {
    yyerrorf("'%s' (typeof '%s') is not struct",
      ref(char, d->name),
      ref(char, m->name));
    return 0;
  }
  return find_struct_member_by_name_(d->member, name);
}

static inline biner_zone_ptr(biner_tree_struct_member_t)
unwrap_struct_member_ref_(
    biner_zone_ptr(biner_tree_struct_member_reference_t) memref) {
  const biner_tree_struct_member_reference_t* r =
    ref(biner_tree_struct_member_reference_t, memref);
  return r->member;
}
