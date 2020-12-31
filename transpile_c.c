#include "./transpile.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "./tree.h"

#define CONTEXT_SUFFIX_ "_pack_context_t"

typedef struct struct_member_type_name_meta_t {
  const char* func;
  const char* type;
} struct_member_type_name_meta_t;

static const struct_member_type_name_meta_t struct_member_type_name_meta_map_
    [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_MAX_] = {
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LU8]  = {"l8",  "uint8_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LU16] = {"l16", "uint16_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LU32] = {"l32", "uint32_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LU64] = {"l64", "uint64_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LI8]  = {"l8",  "int8_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LI16] = {"l16", "int16_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LI32] = {"l32", "int32_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_LI64] = {"l64", "int64_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BU8]  = {"b8",  "uint8_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BU16] = {"b16", "uint16_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BU32] = {"b32", "uint32_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BU64] = {"b64", "uint64_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BI8]  = {"b8",  "int8_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BI16] = {"b16", "int16_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BI32] = {"b32", "int32_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_BI64] = {"b64", "int64_t"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_F16]  = {"f16", "float"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_F32]  = {"f32", "float"},
  [BINER_TREE_STRUCT_MEMBER_TYPE_NAME_F64]  = {"f64", "double"},
};

static const char* const expr_operator_string_map_
    [BINER_TREE_EXPR_TYPE_OPERATOR_MAX_] = {
  [BINER_TREE_EXPR_TYPE_OPERATOR_EQUAL]         = "==",
  [BINER_TREE_EXPR_TYPE_OPERATOR_NEQUAL]        = "!=",
  [BINER_TREE_EXPR_TYPE_OPERATOR_GREATER_EQUAL] = ">=",
  [BINER_TREE_EXPR_TYPE_OPERATOR_LESS_EQUAL]    = "<=",
  [BINER_TREE_EXPR_TYPE_OPERATOR_GREATER]       = ">",
  [BINER_TREE_EXPR_TYPE_OPERATOR_LESS]          = "<",
  [BINER_TREE_EXPR_TYPE_OPERATOR_AND]           = "&&",
  [BINER_TREE_EXPR_TYPE_OPERATOR_OR]            = "||",
  [BINER_TREE_EXPR_TYPE_OPERATOR_NOT]           = "!",
  [BINER_TREE_EXPR_TYPE_OPERATOR_ADD]           = "+",
  [BINER_TREE_EXPR_TYPE_OPERATOR_SUB]           = "-",
  [BINER_TREE_EXPR_TYPE_OPERATOR_MUL]           = "*",
  [BINER_TREE_EXPR_TYPE_OPERATOR_DIV]           = "/",
  [BINER_TREE_EXPR_TYPE_OPERATOR_BIT_AND]       = "&",
  [BINER_TREE_EXPR_TYPE_OPERATOR_BIT_OR]        = "|",
  [BINER_TREE_EXPR_TYPE_OPERATOR_BIT_NOT]       = "~",
  [BINER_TREE_EXPR_TYPE_OPERATOR_BIT_XOR]       = "^",
};

typedef struct struct_member_info_t {
  const biner_transpile_param_t*         p;
  const biner_tree_struct_member_t*      m;
  const biner_tree_struct_member_type_t* t;

  bool   union_begin;
  bool   union_end;
  bool   union_member;
  size_t offset;
} struct_member_info_t;

typedef void (*struct_member_each_func_t)(const struct_member_info_t*);

static size_t struct_member_each_(
    const biner_transpile_param_t*    p,
    const biner_tree_struct_member_t* m,
    struct_member_each_func_t         f,
    size_t                            next_index) {
  assert(p != NULL);
  assert(m != NULL);
  assert(f != NULL);

  struct_member_info_t info = {
    .p           = p,
    .m           = m,
    .t           = (const biner_tree_struct_member_type_t*) (p->zone+m->type),
    .union_begin = (m->index == next_index),
  };

  if (m->prev != 0) {
    const biner_tree_struct_member_t* prev =
      (const biner_tree_struct_member_t*) (p->zone+m->prev);
    if (prev->index == m->index) {
      info.union_end    = !info.union_begin;
      info.union_begin  = false;
      info.union_member = true;
    }
    info.offset = struct_member_each_(p, prev, f, m->index);
  }
  if (info.union_begin) info.union_member = true;

  const size_t size =
    biner_tree_struct_member_type_name_meta_map[info.t->name].size;
  f(&info);
  return info.offset + size;
}

static inline void print_fixed_decl_name_(
    const biner_transpile_param_t* p, const char* name) {
  assert(p    != NULL);
  assert(name != NULL);

  const char* end = name;
  while (*end) ++end;
  if (name+2 <= end && strcmp(end-2, "_t") == 0) end -= 2;
  fprintf(p->dst, "%.*s", (int) (end-name), name);
}

static inline void print_typedef_header_(
    const biner_transpile_param_t* p,
    const char*                    kind,
    const char*                    type,
    const char*                    name) {
  assert(p    != NULL);
  assert(kind != NULL);
  assert(type != NULL);
  assert(name != NULL);

  fprintf(p->dst, "typedef %s ", kind);
  print_fixed_decl_name_(p, type);
  fprintf(p->dst, "%s { ", name);
}

static inline void print_typedef_footer_(
    const biner_transpile_param_t* p,
    const char*                    type,
    const char*                    name) {
  assert(p    != NULL);
  assert(type != NULL);
  assert(name != NULL);

  fprintf(p->dst, "} ");
  print_fixed_decl_name_(p, type);
  fprintf(p->dst, "%s;\n", name);
}

static inline void print_func_header_(
    const biner_transpile_param_t* p,
    const char*                    ret,
    const char*                    type,
    const char*                    name) {
  assert(p    != NULL);
  assert(type != NULL);
  assert(name != NULL);

  fprintf(p->dst, "static inline %s ", ret);
  print_fixed_decl_name_(p, type);
  fprintf(p->dst, "%s(", name);
}

static void print_struct_member_reference_(
    const biner_transpile_param_t*              p,
    const biner_tree_struct_member_reference_t* r) {
  assert(p != NULL);
  assert(r != NULL);

  if (r->prev != 0) {
    print_struct_member_reference_(
      p, (const biner_tree_struct_member_reference_t*) (p->zone+r->prev));
  }
  const biner_tree_struct_member_t* m =
    (const biner_tree_struct_member_t*) (p->zone+r->member);
  fprintf(p->dst, "%s", p->zone+m->name);
  if (r->prev != 0) fprintf(p->dst, ".");
}

static void print_expr_(
    const biner_transpile_param_t* p, const biner_tree_expr_t* e) {
  assert(p != NULL);
  assert(e != NULL);

  fprintf(p->dst, "(");

  switch (e->type) {
  case BINER_TREE_EXPR_TYPE_OPERAND_INTEGER:
    fprintf(p->dst, "%"PRIi64, e->i);
    break;
  case BINER_TREE_EXPR_TYPE_OPERAND_REFERENCE:
    fprintf(p->dst, "s->");
    print_struct_member_reference_(
      p, (const biner_tree_struct_member_reference_t*) (p->zone+e->r));
    break;
  default:
    if (e->operands.l) {
      print_expr_(p, (const biner_tree_expr_t*) (p->zone+e->operands.l));
    }
    fprintf(p->dst, expr_operator_string_map_[e->type]);
    if (e->operands.r) {
      print_expr_(p, (const biner_tree_expr_t*) (p->zone+e->operands.r));
    }
  }

  fprintf(p->dst, ")");
}

static void print_enum_member_decls_(
    const biner_transpile_param_t*  p,
    const biner_tree_enum_member_t* m) {
  assert(p != NULL);
  assert(m != NULL);

  if (m->prev != 0) {
    print_enum_member_decls_(
      p, (const biner_tree_enum_member_t*) (p->zone+m->prev));
  }

  fprintf(p->dst, "%s=", p->zone+m->name);
  print_expr_(p, (const biner_tree_expr_t*) (p->zone+m->expr));
  fprintf(p->dst, ",");
}

static void print_enum_member_validation_code_(
    const biner_transpile_param_t*  p,
    const biner_tree_enum_member_t* m) {
  assert(p != NULL);
  assert(m != NULL);

  fprintf(p->dst, "return true");
  while ((uintptr_t) m != (uintptr_t) p->zone) {
    fprintf(p->dst, " && v == %s", p->zone+m->name);
    m = (const biner_tree_enum_member_t*) (p->zone+m->prev);
  }
  fprintf(p->dst, ";");
}

static void print_struct_member_type_name_(
    const biner_transpile_param_t*         p,
    const biner_tree_struct_member_type_t* t) {
  assert(p != NULL);
  assert(t != NULL);

  if (t->name == BINER_TREE_STRUCT_MEMBER_TYPE_NAME_USER_DECL) {
    const biner_tree_decl_t* d = (const biner_tree_decl_t*) (p->zone+t->decl);
    print_fixed_decl_name_(p, (const char*) (p->zone+d->name));
    fprintf(p->dst, "_t");
    return;
  }

  const char* name = struct_member_type_name_meta_map_[t->name].type;
  assert(name != NULL);
  fprintf(p->dst, "%s", name);
}

static void print_struct_member_decl_(const struct_member_info_t* info) {
  assert(info != NULL);

  if (info->union_begin) fprintf(info->p->dst, "union { ");

  print_struct_member_type_name_(info->p, info->t);
  fprintf(info->p->dst, " ");

  switch (info->t->qualifier) {
  case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_NONE:
    fprintf(info->p->dst, "%s", info->p->zone+info->m->name);
    break;
  case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_STATIC_ARRAY:
    fprintf(info->p->dst, "%s[", info->p->zone+info->m->name);
    print_expr_(
      info->p, (const biner_tree_expr_t*) (info->p->zone+info->t->expr));
    fprintf(info->p->dst, "]");
    break;
  case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_DYNAMIC_ARRAY:
    fprintf(info->p->dst, "*%s", info->p->zone+info->m->name);
    break;
  }
  fprintf(info->p->dst, "; ");
  if (info->union_end) fprintf(info->p->dst, "}; ");
}

static void print_struct_member_context_struct_(
    const biner_transpile_param_t*    p,
    const biner_tree_struct_member_t* m,
    const char*                       suffix) {
  assert(p != NULL);
  assert(m != NULL);

  fprintf(p->dst, "size_t step; ");
  fprintf(p->dst, "size_t count; ");
  fprintf(p->dst, "size_t count_max; ");
  fprintf(p->dst, "size_t byte; ");
  fprintf(p->dst, "void*  udata; ");

  bool require_subctx = false;
  const biner_tree_struct_member_t* itr = m;
  while ((uintptr_t) itr != (uintptr_t) p->zone) {
    const biner_tree_struct_member_type_t* t =
      (const biner_tree_struct_member_type_t*) (p->zone+itr->type);
    if (t->name == BINER_TREE_STRUCT_MEMBER_TYPE_NAME_USER_DECL) {
      require_subctx = true;
    }
    itr = (const biner_tree_struct_member_t*) (p->zone+itr->prev);
  }

  if (require_subctx) {
    fprintf(p->dst, "union { ");

    itr = m;
    while ((uintptr_t) itr != (uintptr_t) p->zone) {
      const biner_tree_struct_member_type_t* t =
        (const biner_tree_struct_member_type_t*) (p->zone+itr->type);
      if (t->name == BINER_TREE_STRUCT_MEMBER_TYPE_NAME_USER_DECL) {
        const biner_tree_decl_t* d = (const biner_tree_decl_t*) (p->zone+t->decl);
        print_fixed_decl_name_(p, (const char*) (p->zone+d->name));
        fprintf(p->dst, "%s %s; ", suffix, p->zone+itr->name);
      }
      itr = (const biner_tree_struct_member_t*) (p->zone+itr->prev);
    }

    fprintf(p->dst, "} subctx; ");
  }
}

static void print_struct_member_pack_code_each_(
    const struct_member_info_t* info) {
  assert(info != NULL);

  const char* name = (const char*) (info->p->zone+info->m->name);

  /* only available when info->p->name == USER_DECL */
  const biner_tree_decl_t* d =
    (const biner_tree_decl_t*) (info->p->zone+info->t->decl);
  const char* dname = (const char*) (info->p->zone+d->name);

  if (!info->union_member || info->union_begin) {
    fprintf(info->p->dst, "init%zu: ", info->m->index);
    fprintf(info->p->dst, "++ctx->step; ctx->byte = 0; ");

    switch (info->t->qualifier) {
    case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_DYNAMIC_ARRAY:
      fprintf(info->p->dst, "ctx->count_max = ");
      print_expr_(
        info->p, (const biner_tree_expr_t*) (info->p->zone+info->t->expr));
      fprintf(info->p->dst, "; ");
      fprintf(info->p->dst,
        "if (ctx->count_max <= 0) { ++ctx->step; goto CONTINUE; } ");
      fprintf(info->p->dst, "ctx->count = 0; ");
      break;
    case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_STATIC_ARRAY:
      fprintf(info->p->dst, "ctx->count = 0; ");
      break;
    default:
      ;
    }
    if (info->t->name == BINER_TREE_STRUCT_MEMBER_TYPE_NAME_USER_DECL) {
      fprintf(info->p->dst, "ctx->subctx.%s = (", name);
      print_fixed_decl_name_(info->p, dname);
      fprintf(info->p->dst, CONTEXT_SUFFIX_") { .udata = ctx->udata, }; ");
    }

    fprintf(info->p->dst, "body%zu: ", info->m->index);
  }

  if (info->m->condition != 0) {
    fprintf(info->p->dst, "if (");
    print_expr_(
      info->p, (const biner_tree_expr_t*) (info->p->zone+info->m->condition));
    fprintf(info->p->dst, ") {");
  }

  const char* suffix = "";
  switch (info->t->qualifier) {
  case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_DYNAMIC_ARRAY:
  case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_STATIC_ARRAY:
    suffix = "[ctx->count]";
    break;
  default:
    ;
  }

  if (info->t->name == BINER_TREE_STRUCT_MEMBER_TYPE_NAME_USER_DECL) {
    fprintf(info->p->dst, "result = ");
    print_fixed_decl_name_(info->p, dname);
    fprintf(info->p->dst, "_pack(&ctx->subctx.%s, &s->%s%s, c); ", name, name, suffix);
    fprintf(info->p->dst,
      "if (result != BINER_RESULT_COMPLETED) { return result; } else { ");
  } else {
    const char* func = struct_member_type_name_meta_map_[info->t->name].func;
    fprintf(info->p->dst,
      "biner_pack_%s(&s->%s%s, c, ctx->byte); ", func, name, suffix);
    const size_t sz = biner_tree_struct_member_type_name_meta_map[info->t->name].size;
    fprintf(info->p->dst, "if (++ctx->byte >= %zu) { ", sz);
  }

  bool reset = false;
  switch (info->t->qualifier) {
  case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_DYNAMIC_ARRAY:
    fprintf(info->p->dst,
      "if (++ctx->count >= ctx->count_max) goto NEXT; ctx->byte = 0; ");
    reset = true;
    break;
  case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_STATIC_ARRAY:
    fprintf(info->p->dst, "if (++ctx->count >= ");
    print_expr_(
      info->p, (const biner_tree_expr_t*) (info->p->zone+info->t->expr));
    fprintf(info->p->dst, ") goto NEXT; ctx->byte = 0; ");
    reset = true;
    break;
  default:
    fprintf(info->p->dst, "goto NEXT; ");
  }
  if (reset) {
    if (info->t->name == BINER_TREE_STRUCT_MEMBER_TYPE_NAME_USER_DECL) {
      fprintf(info->p->dst, "ctx->subctx.%s = (", name);
      print_fixed_decl_name_(info->p, dname);
      fprintf(info->p->dst, CONTEXT_SUFFIX_") { .udata = ctx->udata, }; ");
    }
    fprintf(info->p->dst, "ctx->byte = 0; ");
  }
  fprintf(info->p->dst, "} return BINER_RESULT_CONTINUE; ");

  if (info->m->condition != 0) {
    fprintf(info->p->dst, "} ");
    if (!info->union_member || info->union_end) {
      fprintf(info->p->dst, "goto NEXT; ");
    }
  }
}

static void print_struct_member_unpack_code_each_(
    const struct_member_info_t* info) {
  assert(info != NULL);

  const char* name = (const char*) (info->p->zone+info->m->name);

  /* only available when info->p->name == USER_DECL */
  const biner_tree_decl_t* d =
    (const biner_tree_decl_t*) (info->p->zone+info->t->decl);
  const char* dname = (const char*) (info->p->zone+d->name);

  if (!info->union_member || info->union_begin) {
    fprintf(info->p->dst, "init%zu: ", info->m->index);
    fprintf(info->p->dst, "++ctx->step; ctx->byte = 0; ");

    switch (info->t->qualifier) {
    case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_DYNAMIC_ARRAY:
      fprintf(info->p->dst, "ctx->count_max = ");
      print_expr_(
        info->p, (const biner_tree_expr_t*) (info->p->zone+info->t->expr));
      fprintf(info->p->dst, "; ");
      fprintf(info->p->dst,
        "if (ctx->count_max <= 0) { ++ctx->step; goto CONTINUE; } ");
      fprintf(info->p->dst,
        "s->%s = biner_malloc_(sizeof(*s->%s)*ctx->count_max, ctx->udata); ",
        name, name);
      fprintf(info->p->dst,
        "if (s->%s == NULL) { return BINER_RESULT_MEMORY_ERROR; } ", name);
      fprintf(info->p->dst, "ctx->count = 0; ");
      break;
    case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_STATIC_ARRAY:
      fprintf(info->p->dst, "ctx->count = 0; ");
      break;
    default:
      ;
    }
    if (info->t->name == BINER_TREE_STRUCT_MEMBER_TYPE_NAME_USER_DECL) {
      fprintf(info->p->dst, "ctx->subctx.%s = (", name);
      print_fixed_decl_name_(info->p, dname);
      fprintf(info->p->dst, CONTEXT_SUFFIX_") { .udata = ctx->udata, }; ");
    }

    fprintf(info->p->dst, "body%zu: ", info->m->index);
  }

  if (info->m->condition != 0) {
    fprintf(info->p->dst, "if (");
    print_expr_(
      info->p, (const biner_tree_expr_t*) (info->p->zone+info->m->condition));
    fprintf(info->p->dst, ") {");
  }

  const char* suffix = "";
  switch (info->t->qualifier) {
  case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_DYNAMIC_ARRAY:
  case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_STATIC_ARRAY:
    suffix = "[ctx->count]";
    break;
  default:
    ;
  }

  if (info->t->name == BINER_TREE_STRUCT_MEMBER_TYPE_NAME_USER_DECL) {
    fprintf(info->p->dst, "result = ");
    print_fixed_decl_name_(info->p, dname);
    fprintf(info->p->dst, "_unpack(&ctx->subctx.%s, &s->%s%s, c); ", name, name, suffix);
    fprintf(info->p->dst,
      "if (result != BINER_RESULT_COMPLETED) { return result; } else { ");
  } else {
    const char* func = struct_member_type_name_meta_map_[info->t->name].func;
    fprintf(info->p->dst,
      "biner_unpack_%s(&s->%s%s, c, ctx->byte); ", func, name, suffix);
    const size_t sz = biner_tree_struct_member_type_name_meta_map[info->t->name].size;
    fprintf(info->p->dst, "if (++ctx->byte >= %zu) { ", sz);
  }

  bool reset = false;
  switch (info->t->qualifier) {
  case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_DYNAMIC_ARRAY:
    fprintf(info->p->dst,
      "if (++ctx->count >= ctx->count_max) goto NEXT; ");
    reset = true;
    break;
  case BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_STATIC_ARRAY:
    fprintf(info->p->dst, "if (++ctx->count >= ");
    print_expr_(
      info->p, (const biner_tree_expr_t*) (info->p->zone+info->t->expr));
    fprintf(info->p->dst, ") goto NEXT; ");
    reset = true;
    break;
  default:
    fprintf(info->p->dst, "goto NEXT; ");
  }
  if (reset) {
    if (info->t->name == BINER_TREE_STRUCT_MEMBER_TYPE_NAME_USER_DECL) {
      fprintf(info->p->dst, "ctx->subctx.%s = (", name);
      print_fixed_decl_name_(info->p, dname);
      fprintf(info->p->dst, CONTEXT_SUFFIX_") { .udata = ctx->udata, }; ");
    }
    fprintf(info->p->dst, "ctx->byte = 0; ");
  }
  fprintf(info->p->dst, "} return BINER_RESULT_CONTINUE; ");

  if (info->m->condition != 0) {
    fprintf(info->p->dst, "} ");
    if (!info->union_member || info->union_end) {
      fprintf(info->p->dst, "goto NEXT; ");
    }
  }
}

static inline void print_struct_member_iteration_code_(
    const biner_transpile_param_t*    p,
    const biner_tree_struct_member_t* m,
    struct_member_each_func_t         f) {
  assert(p != NULL);
  assert(m != NULL);
  assert(f != NULL);

  bool require_continue   = false;
  bool require_result_var = false;
  const biner_tree_struct_member_t* itr = m;
  while ((uintptr_t) itr != (uintptr_t) p->zone) {
    const biner_tree_struct_member_type_t* t =
      (const biner_tree_struct_member_type_t*) (p->zone+itr->type);
    if (t->qualifier == BINER_TREE_STRUCT_MEMBER_TYPE_QUALIFIER_DYNAMIC_ARRAY) {
      require_continue = true;
    }
    if (t->name == BINER_TREE_STRUCT_MEMBER_TYPE_NAME_USER_DECL) {
      require_result_var = true;
    }
    itr = (const biner_tree_struct_member_t*) (p->zone+itr->prev);
  }

  fprintf(p->dst, "static const void* const steps_[] = { ");
  for (size_t i = 0; i <= m->index; ++i) {
    fprintf(p->dst, "&&init%zu, &&body%zu, ", i, i);
  }
  fprintf(p->dst, "}; ");

  if (require_result_var) {
    fprintf(p->dst, "biner_result_t result; ");
  }
  if (require_continue) {
    fprintf(p->dst, "CONTINUE: ");
  }
  fprintf(p->dst,
    "if (ctx->step >= sizeof(steps_)/sizeof(steps_[0]))"
    " return BINER_RESULT_COMPLETED; ");
  fprintf(p->dst, "goto *steps_[ctx->step]; ");

  struct_member_each_(p, m, f, SIZE_MAX);

  fprintf(p->dst, "NEXT: ");
  fprintf(p->dst,
    "return (++ctx->step >= sizeof(steps_)/sizeof(steps_[0]))?"
    " BINER_RESULT_COMPLETED:"
    " BINER_RESULT_CONTINUE; ");
}

static void print_decls_(
    const biner_transpile_param_t* p, const biner_tree_decl_t* d) {
  assert(p != NULL);
  assert(d != NULL);

  if (d->prev != 0) {
    print_decls_(p, (const biner_tree_decl_t*) (p->zone+d->prev));
  }

  const char* name = (const char*) (p->zone+d->name);

  switch (d->type) {
  case BINER_TREE_DECL_TYPE_CONST: {
    const biner_tree_expr_t* body =
      (const biner_tree_expr_t*) (p->zone+d->const_);

    fprintf(p->dst, "#define %s (", name);
    print_expr_(p, body);
    fprintf(p->dst, ")\n");
  } break;
  case BINER_TREE_DECL_TYPE_ENUM: {
    const biner_tree_enum_member_t* body =
      (const biner_tree_enum_member_t*) (p->zone+d->enum_);

    print_typedef_header_(p, "enum", name, "_t");
    print_enum_member_decls_(p, body);
    print_typedef_footer_(p, name, "_t");

    print_func_header_(p, "bool", name, "_validate");
    fprintf(p->dst, "uintmax_t v) { ");
    print_enum_member_validation_code_(p, body);
    fprintf(p->dst, "}\n");
  } break;
  case BINER_TREE_DECL_TYPE_STRUCT: {
    const biner_tree_struct_member_t* body =
      (const biner_tree_struct_member_t*) (p->zone+d->struct_);

    print_typedef_header_(p, "struct", name, "_t");
    struct_member_each_(p, body, print_struct_member_decl_, SIZE_MAX);
    print_typedef_footer_(p, name, "_t");

    print_typedef_header_(p, "struct", name, CONTEXT_SUFFIX_);
    print_struct_member_context_struct_(p, body, CONTEXT_SUFFIX_);
    print_typedef_footer_(p, name, CONTEXT_SUFFIX_);

    print_func_header_(p, "biner_result_t", name, "_pack");
    print_fixed_decl_name_(p, name);
    fprintf(p->dst, CONTEXT_SUFFIX_"* ctx, ");
    fprintf(p->dst, "const ");
    print_fixed_decl_name_(p, name);
    fprintf(p->dst, "_t* s, ");
    fprintf(p->dst, "uint8_t* c) { ");
    print_struct_member_iteration_code_(
      p, body, &print_struct_member_pack_code_each_);
    fprintf(p->dst, "}\n");

    print_func_header_(p, "biner_result_t", name, "_unpack");
    print_fixed_decl_name_(p, name);
    fprintf(p->dst, CONTEXT_SUFFIX_"* ctx, ");
    print_fixed_decl_name_(p, name);
    fprintf(p->dst, "_t* s, ");
    fprintf(p->dst, "uint8_t c) { ");
    print_struct_member_iteration_code_(
      p, body, &print_struct_member_unpack_code_each_);
    fprintf(p->dst, "}\n");
  } break;
  }
}

bool biner_transpile_c(const biner_transpile_param_t* p) {
  assert(p != NULL);

  const biner_tree_root_t* root = (const biner_tree_root_t*) p->zone;
  print_decls_(p, (const biner_tree_decl_t*) (p->zone+root->decls));
  return true;
}
