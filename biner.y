%{
#include <assert.h>
#include <stdbool.h>
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

static inline bool
unstringify_struct_member_type_name_(
  biner_tree_struct_member_type_name_t* name,
  biner_zone_ptr(char)                  str
);

static inline biner_zone_ptr(biner_tree_decl_t)
find_decl_by_name_(
  biner_zone_ptr(char) name
);

static inline biner_zone_ptr(biner_tree_struct_member_t)
find_struct_member_by_name_(
  biner_zone_ptr(biner_tree_struct_member_t) last_member,
  biner_zone_ptr(char)                       name
);

static inline biner_zone_ptr(biner_tree_struct_member_t)
find_child_struct_member_by_name_(
  biner_zone_ptr(biner_tree_struct_member_t) member,
  biner_zone_ptr(char)                       name
);

static inline biner_zone_ptr(biner_tree_struct_member_t)
unwrap_struct_member_ref_(
  biner_zone_ptr(biner_tree_struct_member_reference_t) memref
);

static biner_zone_ptr(biner_tree_expr_t)
resolve_constant_(
  biner_zone_ptr(char) ident
);
%}

%union {
  int64_t   i;
  uintptr_t ptr;
}

%token STRUCT
%token <ptr> IDENT
%token <i>   INTEGER;

%type <ptr> decl_list decl
%type <ptr> struct_member_list struct_member
%type <ptr> struct_member_type array_struct_member_type unqualified_struct_member_type
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
  : STRUCT IDENT '{' struct_member_list '}' ';' {
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

struct_member_list
  : struct_member {
    $$ = ctx.last_member = $1;
  }
  | struct_member_list struct_member {
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
  : array_struct_member_type
  | unqualified_struct_member_type
  ;

array_struct_member_type
  : unqualified_struct_member_type '[' expr ']' {
    $$ = $1;
    biner_tree_struct_member_type_t* t =
        ref(biner_tree_struct_member_type_t, $$);
    t->qualifier = BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_DYNAMIC_ARRAY;
    t->expr      = $3;
  }
  ;

unqualified_struct_member_type
  : IDENT {
    $$ = alloc_(biner_tree_struct_member_type_t);
    biner_tree_struct_member_type_t* t =
        ref(biner_tree_struct_member_type_t, $$);
    *t = (biner_tree_struct_member_type_t) {
      .qualifier = BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_NONE,
    };

    const biner_zone_ptr(biner_tree_decl_t) decl = find_decl_by_name_($1);
    if (decl == 0) {
      if (!unstringify_struct_member_type_name_(&t->name, $1)) {
        yyerrorf("unknown type '%s'", ref(char, $1));
        YYABORT;
      }
    } else {
      t->name = BINER_TREE_STRUCT_MEMBER_TYPE_NAME_USER_DECL;
      t->decl = decl;
    }
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
  | IDENT {
    const biner_zone_ptr(biner_tree_struct_member_t) m =
      find_struct_member_by_name_(ctx.last_member, $1);
    if (m != 0) {
      biner_zone_ptr(biner_tree_struct_member_reference_t) mref =
        alloc_(biner_tree_struct_member_reference_t);
      *ref(biner_tree_struct_member_reference_t, mref) =
        (biner_tree_struct_member_reference_t) { .member = m, };

      $$ = alloc_(biner_tree_expr_t);
      *ref(biner_tree_expr_t, $$) = (biner_tree_expr_t) {
        .type = BINER_TREE_EXPR_TYPE_OPERAND_REFERENCE,
        .r    = mref,
      };
    } else {
      $$ = resolve_constant_($1);
      if ($$ == 0) {
        yyerrorf("unknown member or symbol '%s'", ref(char, $1));
        YYABORT;
      }
    }
  }
  | operand '.' IDENT {
    biner_tree_expr_t* expr = ref(biner_tree_expr_t, $1);
    if (expr->type != BINER_TREE_EXPR_TYPE_OPERAND_REFERENCE) {
      yyerrorf("refering non-struct's child, '%s'", ref(char, $3));
      YYABORT;
    }
    const biner_zone_ptr(biner_tree_struct_member_t) m =
      find_child_struct_member_by_name_(unwrap_struct_member_ref_(expr->r), $3);
    if (m == 0) {
      yyerrorf("unknown member '%s'", ref(char, $3));
      YYABORT;
    }
    $$ = alloc_(biner_tree_struct_member_reference_t);
    *ref(biner_tree_struct_member_reference_t, $$) =
      (biner_tree_struct_member_reference_t) {
        .member = m,
        .prev   = expr->r,
      };
    expr->r = $$;
    /* A shared expression such as constant cannot have a reference to member.
     * So modification of the expression doesn't cause any side effects.
     */
  }
  | '(' expr ')' { $$ = $2; }
  ;

%%
static inline bool unstringify_struct_member_type_name_(
    biner_tree_struct_member_type_name_t* name,
    biner_zone_ptr(char)                  str) {
  for (size_t i = 0; i < BINER_TREE_STRUCT_MEMBER_TYPE_NAME_MAX_; ++i) {
    const char* item = biner_tree_struct_member_type_name_string_map[i];
    if (item != NULL && strcmp(ref(char, str), item) == 0) {
      *name = (biner_tree_struct_member_type_name_t) i;
      return true;
    }
  }
  return false;
}

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
  if (t->name != BINER_TREE_STRUCT_MEMBER_TYPE_NAME_USER_DECL) {
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

static inline biner_zone_ptr(biner_tree_expr_t) resolve_constant_(
    biner_zone_ptr(char) ident) {
  (void) ident;
  /* TODO: check out enums and constants */
  return 0;
}
