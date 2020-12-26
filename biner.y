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

static inline biner_zone_ptr(biner_tree_decl_t)
create_decl_(
  biner_zone_ptr(char)   name,
  biner_tree_decl_type_t type,
  biner_zone_ptr(void)   body
);

static inline biner_zone_ptr(biner_tree_decl_t)
find_decl_by_name_(
  biner_zone_ptr(char) name
);

static inline biner_zone_ptr(biner_tree_expr_t)
find_enum_member_by_name_(
  biner_zone_ptr(biner_tree_enum_member_t) tail_member,
  biner_zone_ptr(char)                     name
);

static inline biner_zone_ptr(biner_tree_struct_member_t)
create_struct_member_(
  biner_zone_ptr(biner_tree_expr_t)               condition,
  biner_zone_ptr(biner_tree_struct_member_type_t) type,
  biner_zone_ptr(char)                            name
);

static inline bool
unstringify_struct_member_type_name_(
  biner_tree_struct_member_type_name_t* name,
  biner_zone_ptr(char)                  str
);

static inline biner_zone_ptr(biner_tree_struct_member_t)
find_struct_member_by_name_(
  biner_zone_ptr(biner_tree_struct_member_t) tail_member,
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

static inline biner_zone_ptr(biner_tree_expr_t)
create_operator_(
  biner_zone_ptr(biner_tree_expr_t) l,
  biner_tree_expr_type_t            type,
  biner_zone_ptr(biner_tree_expr_t) r
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

%token EQUAL NEQUAL LESS_EQUAL GREATER_EQUAL
%token CONST ENUM STRUCT UNION
%token <ptr> IDENT
%token <i>   INTEGER;

%type <ptr> decl_list decl
%type <ptr> enum_member_list enum_member
%type <ptr> struct_member_list struct_member union_member_list union_member
%type <ptr> struct_member_type array_struct_member_type unqualified_struct_member_type
%type <ptr> expr compare_expr add_expr mul_expr unary_expr operand

%start decl_list

%%

decl_list
  : decl ';' {
    *ref(biner_tree_root_t, ctx.root) = (biner_tree_root_t) {
      .decls = $1,
    };
    $$ = ctx.root;
  }
  | decl_list decl ';' {
    ref(biner_tree_decl_t, $2)->prev  = ref(biner_tree_root_t, $1)->decls;
    ref(biner_tree_root_t, $1)->decls = $2;
    $$ = $1;
  }
  ;

decl
  : CONST IDENT '=' expr {
    $$ = create_decl_($2, BINER_TREE_DECL_TYPE_CONST, $4);
    if ($$ == 0) YYABORT;
  }
  | ENUM IDENT '{' enum_member_list '}' {
    $$ = create_decl_($2, BINER_TREE_DECL_TYPE_ENUM, $4);
    if ($$ == 0) YYABORT;
  }
  | ENUM IDENT '{' enum_member_list ',' '}' {
    $$ = create_decl_($2, BINER_TREE_DECL_TYPE_ENUM, $4);
    if ($$ == 0) YYABORT;
  }
  | STRUCT IDENT '{' struct_member_list '}' {
    $$ = create_decl_($2, BINER_TREE_DECL_TYPE_STRUCT, $4);
    if ($$ == 0) YYABORT;
  }
  ;

enum_member_list
  : enum_member {
    $$ = ctx.last_enum = $1;
  }
  | enum_member_list ',' enum_member {
    ref(biner_tree_enum_member_t, $3)->prev = $1;
    $$ = ctx.last_enum = $3;
  }
  ;

enum_member
  : IDENT '=' expr {
    if (resolve_constant_($1) != 0) {
      yyerrorf("duplicated symbol name, '%s'", ref(char, $1));
      YYABORT;
    }
    if (ref(biner_tree_expr_t, $3)->dynamic) {
      yyerrorf("dynamic expression is not allowed for enum member, '%s'", ref(char, $1));
      YYABORT;
    }
    $$ = alloc_(biner_tree_enum_member_t);
    *ref(biner_tree_enum_member_t, $$) = (biner_tree_enum_member_t) {
      .name = $1,
      .expr = $3,
    };
  }
  ;

struct_member_list
  : struct_member ';' {
    $$ = ctx.last_struct = $1;
  }
  | struct_member_list struct_member ';' {
    $$ = ctx.last_struct = $2;

    biner_tree_struct_member_t* list = ref(biner_tree_struct_member_t, $1);
    const size_t index = list->index + 1;

    biner_zone_ptr(biner_tree_struct_member_t) itr = $2;
    while (itr) {
      biner_tree_struct_member_t* m = ref(biner_tree_struct_member_t, itr);
      m->index = index;
      itr = m->prev;
      if (itr == 0) m->prev = $1;
    }
  }
  ;

union_member_list
  : union_member ';' {
    $$ = $1;
  }
  | union_member_list union_member ';' {
    $$ = $2;
    ref(biner_tree_struct_member_t, $2)->prev = $1;
  }
  ;

struct_member
  : union_member
  | UNION '{' union_member_list '}' {
    $$ = $3;
  }
  ;

union_member
  : struct_member_type IDENT {
    $$ = create_struct_member_(0, $1, $2);
    if ($$ == 0) YYABORT;
  }
  | '(' expr ')' struct_member_type IDENT {
    $$ = create_struct_member_($2, $4, $5);
    if ($$ == 0) YYABORT;
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
    t->expr = $3;

    const biner_tree_expr_t* expr = ref(biner_tree_expr_t, $3);
    t->qualifier = expr->dynamic?
      BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_DYNAMIC_ARRAY:
      BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_STATIC_ARRAY;
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
  : compare_expr
  ;

compare_expr
  : add_expr
  | compare_expr EQUAL add_expr {
    $$ = create_operator_($1, BINER_TREE_EXPR_TYPE_OPERATOR_EQUAL, $3);
  }
  | compare_expr NEQUAL add_expr {
    $$ = create_operator_($1, BINER_TREE_EXPR_TYPE_OPERATOR_NEQUAL, $3);
  }
  | compare_expr '>' add_expr {
    $$ = create_operator_($1, BINER_TREE_EXPR_TYPE_OPERATOR_GREATER, $3);
  }
  | compare_expr '<' add_expr {
    $$ = create_operator_($1, BINER_TREE_EXPR_TYPE_OPERATOR_LESS, $3);
  }
  | compare_expr GREATER_EQUAL add_expr {
    $$ = create_operator_($1, BINER_TREE_EXPR_TYPE_OPERATOR_GREATER_EQUAL, $3);
  }
  | compare_expr LESS_EQUAL add_expr {
    $$ = create_operator_($1, BINER_TREE_EXPR_TYPE_OPERATOR_LESS_EQUAL, $3);
  }
  ;

add_expr
  : mul_expr
  | add_expr '+' mul_expr {
    $$ = create_operator_($1, BINER_TREE_EXPR_TYPE_OPERATOR_ADD, $3);
  }
  | add_expr '-' mul_expr {
    $$ = create_operator_($1, BINER_TREE_EXPR_TYPE_OPERATOR_SUB, $3);
  }
  ;

mul_expr
  : unary_expr
  | mul_expr '*' unary_expr {
    $$ = create_operator_($1, BINER_TREE_EXPR_TYPE_OPERATOR_MUL, $3);
  }
  | mul_expr '/' unary_expr {
    $$ = create_operator_($1, BINER_TREE_EXPR_TYPE_OPERATOR_DIV, $3);
  }
  ;

unary_expr
  : operand
  | '!' operand {
    $$ = create_operator_($2, BINER_TREE_EXPR_TYPE_OPERATOR_NOT, 0);
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
      find_struct_member_by_name_(ctx.last_struct, $1);
    if (m != 0) {
      biner_zone_ptr(biner_tree_struct_member_reference_t) mref =
        alloc_(biner_tree_struct_member_reference_t);
      *ref(biner_tree_struct_member_reference_t, mref) =
        (biner_tree_struct_member_reference_t) { .member = m, };

      $$ = alloc_(biner_tree_expr_t);
      *ref(biner_tree_expr_t, $$) = (biner_tree_expr_t) {
        .type    = BINER_TREE_EXPR_TYPE_OPERAND_REFERENCE,
        .dynamic = true,
        .r       = mref,
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
    biner_zone_ptr(biner_tree_struct_member_reference_t) r =
      alloc_(biner_tree_struct_member_reference_t);
    *ref(biner_tree_struct_member_reference_t, r) =
      (biner_tree_struct_member_reference_t) {
        .member = m,
        .prev   = expr->r,
      };
    expr->r = r;
    $$ = $1;
    /* A shared expression such as constant cannot have a reference to member.
     * So modification of the expression doesn't cause any side effects.
     */
  }
  | '(' expr ')' { $$ = $2; }
  ;

%%
static inline biner_zone_ptr(biner_tree_decl_t) create_decl_(
    biner_zone_ptr(char)   name,
    biner_tree_decl_type_t type,
    biner_zone_ptr(void)   body) {
  if (resolve_constant_(name) != 0) {
    yyerrorf("duplicated symbol name, '%s'", ref(char, name));
    return 0;
  }
  if (find_decl_by_name_(name) != 0) {
    yyerrorf("duplicated declaration of '%s'", ref(char, name));
    return 0;
  }
  biner_zone_ptr(biner_tree_decl_t) decl = alloc_(biner_tree_decl_t);
  *ref(biner_tree_decl_t, decl) = (biner_tree_decl_t) {
    .name = name,
    .type = type,
    .body = body,
  };
  ctx.last_decl   = decl;
  ctx.last_enum   = 0;
  ctx.last_struct = 0;
  return decl;
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

static inline biner_zone_ptr(biner_tree_struct_member_t) create_struct_member_(
    biner_zone_ptr(biner_tree_expr_t)               condition,
    biner_zone_ptr(biner_tree_struct_member_type_t) type,
    biner_zone_ptr(char)                            name) {
  if (resolve_constant_(name) != 0) {
    yyerrorf("duplicated symbol name, '%s'", ref(char, name));
    return 0;
  }
  if (find_struct_member_by_name_(ctx.last_struct, name) != 0) {
    yyerrorf("duplicated struct member of '%s'", ref(char, name));
    return 0;
  }
  const biner_zone_ptr(biner_tree_struct_member_t) ret =
    alloc_(biner_tree_struct_member_t);
  *ref(biner_tree_struct_member_t, ret) =
    (biner_tree_struct_member_t) {
      .type      = type,
      .condition = condition,
      .name      = name,
    };
  return ret;
}

static inline biner_zone_ptr(biner_tree_expr_t) find_enum_member_by_name_(
  biner_zone_ptr(biner_tree_enum_member_t) tail_member,
  biner_zone_ptr(char)                     name) {
  biner_zone_ptr(biner_tree_enum_member_t) itr = tail_member;
  while (itr) {
    const biner_tree_enum_member_t* member = ref(biner_tree_enum_member_t, itr);
    if (strcmp(ref(char, member->name), ref(char, name)) == 0) {
      return member->expr;
    }
    itr = member->prev;
  }
  return 0;
}

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

static inline biner_zone_ptr(biner_tree_struct_member_t)
find_struct_member_by_name_(
    biner_zone_ptr(biner_tree_struct_member_t) tail_member,
    biner_zone_ptr(char)                       name) {
  biner_zone_ptr(biner_tree_struct_member_t) itr = tail_member;
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
  return find_struct_member_by_name_(d->struct_, name);
}

static inline biner_zone_ptr(biner_tree_struct_member_t)
unwrap_struct_member_ref_(
    biner_zone_ptr(biner_tree_struct_member_reference_t) memref) {
  const biner_tree_struct_member_reference_t* r =
    ref(biner_tree_struct_member_reference_t, memref);
  return r->member;
}

static inline biner_zone_ptr(biner_tree_expr_t) create_operator_(
    biner_zone_ptr(biner_tree_expr_t) l,
    biner_tree_expr_type_t            type,
    biner_zone_ptr(biner_tree_expr_t) r) {
  const biner_tree_expr_t* lexpr = ref(biner_tree_expr_t, l);
  const biner_tree_expr_t* rexpr = ref(biner_tree_expr_t, r);

  biner_zone_ptr(biner_tree_expr_t) expr = alloc_(biner_tree_expr_t);
  *ref(biner_tree_expr_t, expr) = (biner_tree_expr_t) {
    .type     = type,
    .dynamic  = (l && lexpr->dynamic) || (r && rexpr->dynamic),
    .operands = {l, r},
  };
  return expr;
}

static inline biner_zone_ptr(biner_tree_expr_t) resolve_constant_(
    biner_zone_ptr(char) ident) {
  biner_zone_ptr(biner_tree_expr_t) expr =
    find_enum_member_by_name_(ctx.last_enum, ident);
  if (expr != 0) return expr;

  biner_zone_ptr(biner_tree_decl_t) itr = ctx.last_decl;
  while (itr) {
    const biner_tree_decl_t* decl = ref(biner_tree_decl_t, itr);
    switch (decl->type) {
    case BINER_TREE_DECL_TYPE_CONST:
      if (strcmp(ref(char, decl->name), ref(char, ident)) == 0) {
        expr = decl->const_;
      }
      break;
    case BINER_TREE_DECL_TYPE_ENUM:
      expr = find_enum_member_by_name_(decl->enum_, ident);
      break;
    default:
      ;
    }
    if (expr != 0) return expr;
    itr = decl->prev;
  }
  return 0;
}
