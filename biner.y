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

#define alloc_(T)  (biner_zone_alloc(&ctx.zone, sizeof(T)))
#define ref(T, p) ((T*) (ctx.zone.ptr+p))

extern int  yylex(void);
extern void yyerror(const char*);

static inline biner_zone_ptr(biner_tree_struct_member_t)
find_struct_member_(
    biner_zone_ptr(biner_tree_struct_member_t) itr,
    biner_zone_ptr(char)                       name) {
  while (itr) {
    const biner_tree_struct_member_t* m = ref(biner_tree_struct_member_t, itr);
    if (strcmp(ref(char, m->name), ref(char, name)) == 0) {
      return itr;
    }
    itr = m->prev;
  }
  yyerror("unknown member");
  return 0;
}
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
        .generic      = $1,
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
    $$ = alloc_(biner_tree_struct_member_reference_t);
    *ref(biner_tree_struct_member_reference_t, $$) =
      (biner_tree_struct_member_reference_t) {
        .member = find_struct_member_(ctx.last_member, $1),
      };
  }
  | struct_member_reference '.' IDENT {
    const biner_tree_struct_member_t* p =
      ref(biner_tree_struct_member_t, $1);

    const biner_tree_struct_member_type_t* t =
      ref(biner_tree_struct_member_type_t, p->type);
    if (t->kind != BINER_TREE_STRUCT_MEMBER_TYPE_KIND_USER_DEFINED) {
      yyerror("not user-defined data");
      YYABORT;
    }

    const biner_tree_decl_t* d = ref(biner_tree_decl_t, t->decl);
    if (d->type != BINER_TREE_DECL_TYPE_STRUCT) {
      yyerror("not struct");
      YYABORT;
    }
    $$ = alloc_(biner_tree_struct_member_reference_t);
    *ref(biner_tree_struct_member_reference_t, $$) =
      (biner_tree_struct_member_reference_t) {
        .member = find_struct_member_(d->member, $3),
      };
  }
  ;

%%
