#include <stdlib.h>
#include <string.h>
#define GTKML_INCLUDE_INTERNAL
#include "gtk-ml.h"
#include "gtk-ml-internal.h"

#define gtk_ml_add_builder(b, _builder_name, builder, _require_intrinsic, _require_macro, _require_runtime) \
    do { \
        if (b->len_builder >= b->cap_builder) { \
            b->cap_builder *= 2; \
            b->builders = realloc(b->builders, sizeof(GtkMl_BuilderMacro) * b->cap_builder); \
        } \
        char *_name = _builder_name; \
        char *name = malloc(strlen(_name) + 1); \
        strcpy(name, _name); \
        b->builders[b->len_builder].name = name; \
        b->builders[b->len_builder].fn = builder; \
        b->builders[b->len_builder].require_intrinsic = _require_intrinsic; \
        b->builders[b->len_builder].require_macro = _require_macro; \
        b->builders[b->len_builder].require_runtime = _require_runtime; \
        ++b->len_builder; \
    } while (0);

GtkMl_Builder *gtk_ml_new_builder(GtkMl_Context *ctx) {
    GtkMl_Builder *b = malloc(sizeof(GtkMl_Builder));

    b->basic_blocks = malloc(sizeof(GtkMl_BasicBlock *) * 64);
    b->len_bb = 0;
    b->cap_bb = 64;

    b->data = malloc(sizeof(GtkMl_TaggedValue) * 64);
    b->data[0] = gtk_ml_value_none();
    b->len_data = 1;
    b->cap_data = 64;

    b->statics = malloc(sizeof(GtkMl_SObj) * 64);
    b->statics[0] = NULL;
    b->len_static = 1;
    b->cap_static = 64;

    b->counter = gtk_ml_new_var(ctx, NULL, gtk_ml_new_int(ctx, NULL, 0));
    b->flags = GTKML_F_NONE;

    gtk_ml_push(ctx, gtk_ml_value_sobject(b->counter));

    b->intr_ctx = gtk_ml_new_context_with_gc(gtk_ml_gc_copy(ctx->gc));
    b->intr_ctx->dbg_done = 1; // fake done sending dbg data
    b->macro_ctx = gtk_ml_new_context_with_gc(gtk_ml_gc_copy(ctx->gc));
    b->macro_ctx->dbg_done = 1; // fake done sending dbg data

    b->builders = malloc(sizeof(GtkMl_BuilderMacro) * 64);
    b->len_builder = 0;
    b->cap_builder = 64;

    gtk_ml_add_builder(b, "compile-expr", gtk_ml_builder_compile_expr, 1, 0, 0);
    gtk_ml_add_builder(b, "emit-bytecode", gtk_ml_builder_emit_bytecode, 1, 0, 0);
    gtk_ml_add_builder(b, "bind-symbol", gtk_ml_builder_bind_symbol, 1, 0, 0);
    gtk_ml_add_builder(b, "export-symbol", gtk_ml_builder_export_symbol, 1, 0, 0);
    gtk_ml_add_builder(b, "append-basic-block", gtk_ml_builder_append_basic_block, 1, 0, 0);
    gtk_ml_add_builder(b, "global-counter", gtk_ml_builder_global_counter, 1, 0, 0);
    gtk_ml_add_builder(b, "basic-block-name", gtk_ml_builder_basic_block_name, 1, 0, 0);
    gtk_ml_add_builder(b, "string->symbol", gtk_ml_builder_string_to_symbol, 0, 0, 0);
    gtk_ml_add_builder(b, "do", gtk_ml_builder_do, 0, 0, 0);
    gtk_ml_add_builder(b, "let", gtk_ml_builder_let, 0, 0, 0);
    gtk_ml_add_builder(b, "let*", gtk_ml_builder_let_star, 0, 0, 0);
    gtk_ml_add_builder(b, "lambda", gtk_ml_builder_lambda, 0, 0, 0);
    gtk_ml_add_builder(b, "macro", gtk_ml_builder_macro, 0, 1, 0);
    gtk_ml_add_builder(b, "cond", gtk_ml_builder_cond, 0, 0, 0);
    gtk_ml_add_builder(b, "while", gtk_ml_builder_while, 0, 0, 0);
    gtk_ml_add_builder(b, "len", gtk_ml_builder_len, 0, 0, 0);
    gtk_ml_add_builder(b, "car", gtk_ml_builder_car, 0, 0, 0);
    gtk_ml_add_builder(b, "cdr", gtk_ml_builder_cdr, 0, 0, 0);
    gtk_ml_add_builder(b, "typeof", gtk_ml_builder_typeof, 0, 0, 0);
    gtk_ml_add_builder(b, "->sobject", gtk_ml_builder_to_sobject, 0, 0, 0);
    gtk_ml_add_builder(b, "->prim", gtk_ml_builder_to_prim, 0, 0, 0);
    gtk_ml_add_builder(b, "index", gtk_ml_builder_index, 0, 0, 0);
    gtk_ml_add_builder(b, "push", gtk_ml_builder_push, 0, 0, 0);
    gtk_ml_add_builder(b, "pop", gtk_ml_builder_pop, 0, 0, 0);
    gtk_ml_add_builder(b, "concat", gtk_ml_builder_concat, 0, 0, 0);
    gtk_ml_add_builder(b, "+", gtk_ml_builder_add, 0, 0, 0);
    gtk_ml_add_builder(b, "-", gtk_ml_builder_sub, 0, 0, 0);
    gtk_ml_add_builder(b, "*", gtk_ml_builder_mul, 0, 0, 0);
    gtk_ml_add_builder(b, "/", gtk_ml_builder_div, 0, 0, 0);
    gtk_ml_add_builder(b, "%", gtk_ml_builder_mod, 0, 0, 0);
    gtk_ml_add_builder(b, "bit-not", gtk_ml_builder_bitnot, 0, 0, 0);
    gtk_ml_add_builder(b, "bit-and", gtk_ml_builder_bitand, 0, 0, 0);
    gtk_ml_add_builder(b, "bit-or", gtk_ml_builder_bitor, 0, 0, 0);
    gtk_ml_add_builder(b, "bit-xor", gtk_ml_builder_bitxor, 0, 0, 0);
    gtk_ml_add_builder(b, "cmp", gtk_ml_builder_cmp, 0, 0, 0);
    gtk_ml_add_builder(b, "var", gtk_ml_builder_var, 0, 0, 0);
    gtk_ml_add_builder(b, "vararg", gtk_ml_builder_vararg, 0, 0, 0);
    gtk_ml_add_builder(b, "quote", gtk_ml_builder_quote, 0, 0, 0);
    gtk_ml_add_builder(b, "quasiquote", gtk_ml_builder_quasiquote, 0, 0, 0);
    gtk_ml_add_builder(b, "unquote", gtk_ml_builder_unquote, 0, 0, 0);
    gtk_ml_add_builder(b, "define", gtk_ml_builder_define, 0, 0, 0);
    gtk_ml_add_builder(b, "define-macro", gtk_ml_builder_define_macro, 0, 0, 0);
    gtk_ml_add_builder(b, "define-intrinsic", gtk_ml_builder_define_intrinsic, 0, 0, 0);
    gtk_ml_add_builder(b, "intr-apply", gtk_ml_builder_intr_apply, 1, 0, 0);
#ifdef GTKML_ENABLE_GTK
    gtk_ml_add_builder(b, "Application", gtk_ml_builder_application, 0, 0, 1);
    gtk_ml_add_builder(b, "new-window", gtk_ml_builder_new_window, 0, 0, 1);
#endif /* GTKML_ENABLE_GTK */
    gtk_ml_add_builder(b, "setmetamap", gtk_ml_builder_setmetamap, 0, 0, 0);
    gtk_ml_add_builder(b, "getmetamap", gtk_ml_builder_getmetamap, 0, 0, 0);
    gtk_ml_add_builder(b, "get", gtk_ml_builder_getvar, 0, 0, 0);
    gtk_ml_add_builder(b, "assign", gtk_ml_builder_assignvar, 0, 0, 0);
    gtk_ml_add_builder(b, "error", gtk_ml_builder_error, 0, 0, 0);
    gtk_ml_add_builder(b, "dbg", gtk_ml_builder_dbg, 0, 0, 0);
#ifdef GTKML_ENABLE_POSIX
    gtk_ml_add_builder(b, "dbg-run", gtk_ml_builder_dbg_run, 0, 0, 1);
    gtk_ml_add_builder(b, "dbg-cont", gtk_ml_builder_dbg_cont, 0, 0, 1);
    gtk_ml_add_builder(b, "dbg-step", gtk_ml_builder_dbg_step, 0, 0, 1);
    gtk_ml_add_builder(b, "dbg-disasm", gtk_ml_builder_dbg_disasm, 0, 0, 1);
    gtk_ml_add_builder(b, "dbg-stack", gtk_ml_builder_dbg_stack, 0, 0, 1);
    gtk_ml_add_builder(b, "dbg-backtrace", gtk_ml_builder_dbg_backtrace, 0, 0, 1);
#endif /* GTKML_ENABLE_POSIX */

    gtk_ml_new_hash_set(&b->intr_fns, &GTKML_DEFAULT_HASHER);
    gtk_ml_new_hash_set(&b->macro_fns, &GTKML_DEFAULT_HASHER);

    b->len_base = 1;
    b->cap_base = 64;
    b->base = malloc(sizeof(int64_t) * b->cap_base);
    b->base[0] = 0;

    GtkMl_SObj scope = gtk_ml_new_map(ctx, NULL, NULL);
    b->bindings = gtk_ml_new_array(ctx, NULL);
    GtkMl_SObj tmp = gtk_ml_new_array(ctx, NULL);
    gtk_ml_del_array_trie(ctx, &tmp->value.s_array.array, gtk_ml_delete_value);
    gtk_ml_array_trie_push(&tmp->value.s_array.array, &b->bindings->value.s_array.array, gtk_ml_value_sobject(scope));
    b->bindings = tmp;

    ctx->gc->builder = b;

    return b;
}

void gtk_ml_builder_set_cond(GtkMl_Builder *b, unsigned int flags) {
    b->flags = flags;
}

unsigned int gtk_ml_builder_clear_cond(GtkMl_Builder *b) {
    unsigned int flags = b->flags;
    b->flags = 0;
    return flags;
}

GtkMl_SObj gtk_ml_builder_get_and_inc(GtkMl_Context *ctx, GtkMl_Builder *b) {
    GtkMl_SObj value = b->counter->value.s_var.expr;
    b->counter->value.s_var.expr = gtk_ml_new_int(ctx, NULL, value->value.s_int.value + 1);
    return value;
}

int64_t gtk_ml_builder_alloca(GtkMl_Context *ctx, GtkMl_Builder *b) {
    (void) ctx;
    int64_t offset = b->base[b->len_base - 1];
    ++b->base[b->len_base - 1];
    return offset;
}

void gtk_ml_builder_bind(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_SObj key, int64_t offset) {
    GtkMl_SObj tmp1 = gtk_ml_new_array(ctx, NULL);
    gtk_ml_del_array_trie(ctx, &tmp1->value.s_array.array, gtk_ml_delete_value);
    GtkMl_SObj scope = gtk_ml_array_trie_pop(&tmp1->value.s_array.array, &b->bindings->value.s_array.array).value.sobj;

    GtkMl_SObj tmp2 = gtk_ml_new_map(ctx, NULL, NULL);
    gtk_ml_del_hash_trie(ctx, &tmp2->value.s_map.map, gtk_ml_delete_value);
    gtk_ml_hash_trie_insert(&tmp2->value.s_map.map, &scope->value.s_map.map, gtk_ml_value_sobject(key), gtk_ml_value_int(offset));

    GtkMl_SObj tmp3 = gtk_ml_new_array(ctx, NULL);
    gtk_ml_del_array_trie(ctx, &tmp3->value.s_array.array, gtk_ml_delete_value);
    gtk_ml_array_trie_push(&tmp3->value.s_array.array, &tmp1->value.s_array.array, gtk_ml_value_sobject(tmp2));

    b->bindings = tmp3;
}

GtkMl_TaggedValue gtk_ml_builder_get(GtkMl_Builder *b, GtkMl_SObj _key) {
    GtkMl_TaggedValue key = gtk_ml_value_sobject(_key);
    size_t len = gtk_ml_array_trie_len(&b->bindings->value.s_array.array);
    GtkMl_SObj scope = gtk_ml_array_trie_get(&b->bindings->value.s_array.array, len - 1).value.sobj;
    return gtk_ml_hash_trie_get(&scope->value.s_map.map, key);
}

struct InheritData {
    GtkMl_Context *ctx;
    GtkMl_Builder *b;
    GtkMl_SObj out;
};

GTKML_PRIVATE GtkMl_VisitResult visit_inherit(GtkMl_HashTrie *ht, GtkMl_TaggedValue key, GtkMl_TaggedValue value, GtkMl_TaggedValue _data) {
    struct InheritData *data = _data.value.userdata;

    int64_t len = gtk_ml_hash_trie_len(ht);
    (void) len;
    int64_t offset = -(data->b->base[data->b->len_base - 1] - value.value.s64);
    GtkMl_SObj tmp = gtk_ml_new_map(data->ctx, NULL, NULL);
    gtk_ml_hash_trie_insert(&tmp->value.s_map.map, &data->out->value.s_map.map, key, gtk_ml_value_int(offset));
    gtk_ml_del_hash_trie(data->ctx, &data->out->value.s_map.map, gtk_ml_delete_value);
    data->out = tmp;

    return GTKML_VISIT_RECURSE;
}

void gtk_ml_builder_enter(GtkMl_Context *ctx, GtkMl_Builder *b, gboolean inherit) {
    GtkMl_SObj newscope;
    if (inherit) {
        GtkMl_SObj scope = gtk_ml_array_trie_get(&b->bindings->value.s_array.array, gtk_ml_array_trie_len(&b->bindings->value.s_array.array) - 1).value.sobj;
        GtkMl_SObj tmp1 = gtk_ml_new_map(ctx, NULL, NULL);
        struct InheritData data = { ctx, b, tmp1 };
        gtk_ml_hash_trie_foreach(&scope->value.s_map.map, visit_inherit, gtk_ml_value_userdata(&data));
        newscope = data.out;
    } else {
        newscope = gtk_ml_new_map(ctx, NULL, NULL);
    }
    GtkMl_SObj tmp2 = gtk_ml_new_array(ctx, NULL);
    gtk_ml_array_trie_push(&tmp2->value.s_array.array, &b->bindings->value.s_array.array, gtk_ml_value_sobject(newscope));
    b->bindings = tmp2;

    if (b->len_base == b->cap_base) {
        b->cap_base *= 2;
        b->base = realloc(b->base, sizeof(int64_t) * b->cap_base);
    }
    b->base[b->len_base++] = 0;
}

void gtk_ml_builder_leave(GtkMl_Context *ctx, GtkMl_Builder *b) {
    GtkMl_SObj tmp = gtk_ml_new_array(ctx, NULL);
    gtk_ml_del_array_trie(ctx, &tmp->value.s_array.array, gtk_ml_delete_value);
    gtk_ml_array_trie_pop(&tmp->value.s_array.array, &b->bindings->value.s_array.array);
    b->bindings = tmp;
    --b->len_base;
}

gboolean gtk_ml_build_halt(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_HALT;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_push_imm(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err, GtkMl_Data data) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_PUSH_IMM;
    basic_block->text[basic_block->len_text].data = data;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_setf_imm(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err, GtkMl_Data data) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_SETF_IMM;
    basic_block->text[basic_block->len_text].data = data;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_popf(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_POPF;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_push_addr(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err, GtkMl_Data data) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_EXTERN;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_PUSH_IMM;
    basic_block->text[basic_block->len_text].data = data;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_pop(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_POP;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_define(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_DEFINE;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_car(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_CAR;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_cdr(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_CDR;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_list(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_LIST;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_bind(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_BIND;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_bind_args(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_ENTER_BIND_ARGS;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_local_imm(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err, GtkMl_Data data) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_LOCAL_IMM;
    basic_block->text[basic_block->len_text].data = data;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_enter(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_ENTER;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_leave(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_LEAVE;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_unwrap(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_UNWRAP;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_typeof(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_TYPEOF;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_to_sobject(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_TO_SOBJ;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_to_prim(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_TO_PRIM;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_get_imm(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err, GtkMl_Data data) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_GET_IMM;
    basic_block->text[basic_block->len_text].data = data;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_list_imm(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err, GtkMl_Data data) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_LIST_IMM;
    basic_block->text[basic_block->len_text].data = data;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_map_imm(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err, GtkMl_Data data) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_MAP_IMM;
    basic_block->text[basic_block->len_text].data = data;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_set_imm(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err, GtkMl_Data data) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_SET_IMM;
    basic_block->text[basic_block->len_text].data = data;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_array_imm(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err, GtkMl_Data data) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_ARRAY_IMM;
    basic_block->text[basic_block->len_text].data = data;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_setmm_imm(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_SETMM_IMM;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_getmm_imm(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_GETMM_IMM;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_var(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_VAR;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_getvar(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_GETVAR;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_assignvar(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_ASSIGNVAR;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_len(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_LEN;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_array_index(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_ARRAY_INDEX;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_array_push(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_ARRAY_PUSH;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_array_pop(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_ARRAY_POP;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_array_concat(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_ARRAY_CONCAT;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_map_get(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_MAP_GET;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_map_insert(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_MAP_INSERT;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_map_delete(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_MAP_DELETE;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_set_contains(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_SET_CONTAINS;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_set_insert(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_SET_INSERT;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_set_delete(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_SET_DELETE;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_call_core(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err, GtkMl_Data data) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_CALL_CORE;
    basic_block->text[basic_block->len_text].data = data;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_call(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_CALL;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_branch_absolute(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err, GtkMl_Data data) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_EXTERN;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_BRANCH_ABSOLUTE;
    basic_block->text[basic_block->len_text].data = data;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_ret(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_LEAVE_RET;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_add(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_ADD;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_sub(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_SUBTRACT;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_mul(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_SIGNED_MULTIPLY;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_div(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_SIGNED_DIVIDE;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_mod(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_SIGNED_MODULO;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_cmp(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err, GtkMl_Data data) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_CMP_IMM;
    basic_block->text[basic_block->len_text].data = data;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_bitand(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_BIT_AND;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_bitor(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_BIT_AND;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

gboolean gtk_ml_build_bitxor(GtkMl_Context *ctx, GtkMl_Builder *b, GtkMl_BasicBlock *basic_block, GtkMl_SObj *err) {
    (void) ctx;
    (void) err;

    if (basic_block->len_text == basic_block->cap_text) {
        basic_block->cap_text *= 2;
        basic_block->text = realloc(basic_block->text, sizeof(GtkMl_Instruction) * basic_block->cap_text);
    }

    basic_block->text[basic_block->len_text].cond = gtk_ml_builder_clear_cond(b);
    basic_block->text[basic_block->len_text].category = GTKML_I_GENERIC;
    basic_block->text[basic_block->len_text].opcode = GTKML_I_BIT_XOR;
    basic_block->text[basic_block->len_text].data = 0;
    ++basic_block->len_text;

    return 1;
}

GtkMl_BasicBlock *gtk_ml_append_basic_block(GtkMl_Builder *b, const char *name) {
    if (b->len_bb == b->cap_bb) {
        b->cap_bb *= 2;
        b->basic_blocks = realloc(b->basic_blocks, sizeof(GtkMl_BasicBlock *) * b->cap_bb);
    }

    GtkMl_BasicBlock *basic_block = malloc(sizeof(GtkMl_BasicBlock));
    b->basic_blocks[b->len_bb] = basic_block;
    ++b->len_bb;

    basic_block->name = name;
    basic_block->text = malloc(sizeof(GtkMl_Instruction) * 256);
    basic_block->len_text = 0;
    basic_block->cap_text = 256;

    return basic_block;
}

GtkMl_Data gtk_ml_append_data(GtkMl_Builder *b, GtkMl_TaggedValue value) {
    for (GtkMl_Data i = 0; i < b->len_data; i++) {
        if (b->data[i].tag == value.tag && b->data[i].value.u64 == value.value.u64) {
            return i;
        }
    }

    if (b->len_data == b->cap_data) {
        b->cap_data *= 2;
        b->data = realloc(b->data, sizeof(GtkMl_TaggedValue) * b->cap_data);
    }

    GtkMl_Data handle = b->len_data;
    b->data[handle] = value;
    ++b->len_data;

    return handle;
}

GtkMl_Static gtk_ml_append_static(GtkMl_Builder *b, GtkMl_SObj value) {
    for (GtkMl_Static i = 0; i < b->len_static; i++) {
        if (b->statics[i] == value) {
            return i;
        }
    }

    if (b->len_static == b->cap_static) {
        b->cap_static *= 2;
        b->statics = realloc(b->statics, sizeof(GtkMl_SObj) * b->cap_static);
    }

    GtkMl_Static handle = b->len_static;
    b->statics[handle] = value;
    ++b->len_static;

    return handle;
}

GTKML_PRIVATE GtkMl_Program *build(GtkMl_Context *ctx, GtkMl_SObj *err, GtkMl_Builder *b, GtkMl_Stage stage, gboolean complete) {
    if (ctx->gc->program_len == ctx->gc->program_cap) {
        ctx->gc->program_cap *= 2;
        ctx->gc->programs = realloc(ctx->gc->programs, sizeof(GtkMl_Program *) * ctx->gc->program_cap);
    }
    ctx->gc->programs[ctx->gc->program_len] = malloc(sizeof(GtkMl_Program));
    GtkMl_Program *out = ctx->gc->programs[ctx->gc->program_len++];

    size_t n = 0;
    size_t n_static = b->len_static;
    size_t n_data = b->len_data;
    for (size_t i = 0; i < b->len_bb; i++) {
        n += b->basic_blocks[i]->len_text;
    }

    if (!complete) {
        n += 1;
    }

    GtkMl_Instruction *result = malloc(sizeof(GtkMl_Instruction) * n);
    GtkMl_TaggedValue *data = malloc(sizeof(GtkMl_TaggedValue) * n_data);
    memcpy(data, b->data, sizeof(GtkMl_TaggedValue) * n_data);
    GtkMl_SObj *statics = malloc(sizeof(GtkMl_SObj) * n_static);
    memcpy(statics, b->statics, sizeof(GtkMl_SObj) * n_static);
    uint64_t pc = 0;

    for (size_t i = 0; i < b->len_bb; i++) {
        size_t n = b->basic_blocks[i]->len_text;
        memcpy(result + pc, b->basic_blocks[i]->text, sizeof(GtkMl_Instruction) * n);
        pc += n;

        // add a halt instruction to the end of _start
        if (!complete && i == 0) {
            result[pc].cond = GTKML_F_NONE;
            result[pc].category = GTKML_I_GENERIC;
            result[pc].opcode = GTKML_I_HALT;
            result[pc].data = 0;
            ++pc;
        }
    }

    for (size_t i = 0; i < n; i++) {
        GtkMl_Instruction instr = result[i];
        if (instr.category & GTKML_I_EXPORT) {
            GtkMl_SObj addr = statics[data[result[i].data].value.u64];
            if (addr->kind == GTKML_S_PROGRAM) {
                addr->value.s_program.addr = 8 * i;
            } else if (addr->kind == GTKML_S_ADDRESS) {
                addr->value.s_address.addr = 8 * i;
            } else {
                *err = gtk_ml_error(ctx, "export-error", GTKML_ERR_EXPORT_ERROR, 0, 0, 0, 2,
                    gtk_ml_new_keyword(ctx, NULL, 0, "pc", strlen("pc")), gtk_ml_new_int(ctx, NULL, 8 * i),
                    gtk_ml_new_keyword(ctx, NULL, 0, "got", strlen("got")), addr);
                return NULL;
            }
        }
    }

    for (size_t i = 0; i < n; i++) {
        GtkMl_Instruction instr = result[i];
        if (instr.category & GTKML_I_EXTERN) {
            GtkMl_SObj ext = statics[data[instr.data].value.u64];
            if (ext->kind != GTKML_S_ARRAY || !gtk_ml_array_trie_is_string(&ext->value.s_array.array)) {
                *err = gtk_ml_error(ctx, "type-error", GTKML_ERR_TYPE_ERROR, 0, 0, 0, 2, gtk_ml_new_keyword(ctx, NULL, 0, "expected", strlen("expected")), gtk_ml_new_keyword(ctx, NULL, 0, "string", strlen("string")), gtk_ml_new_keyword(ctx, NULL, 0, "got", strlen("got")), ext);
                return NULL;
            }
            gboolean found = 0;
            GtkMl_Array all_exports;
            gtk_ml_new_array_trie(&all_exports);

            for (size_t l = 0; l < n; l++) {
                GtkMl_Instruction instr = result[l];
                if (instr.category & GTKML_I_EXPORT) {
                    GtkMl_SObj addr = statics[data[instr.data].value.u64];
                    GtkMl_SObj exp;
                    if (addr->kind == GTKML_S_PROGRAM) {
                        exp = addr->value.s_program.linkage_name;
                    } else if (addr->kind == GTKML_S_ADDRESS) {
                        exp = addr->value.s_address.linkage_name;
                    } else {
                        *err = gtk_ml_error(ctx, "export-error", GTKML_ERR_EXPORT_ERROR, 0, 0, 0, 1, gtk_ml_new_keyword(ctx, NULL, 0, "got", strlen("got")), addr);
                        return NULL;
                    }
                    GtkMl_Array new;
                    gtk_ml_array_trie_push(&new, &all_exports, gtk_ml_value_sobject(exp));
                    gtk_ml_del_array_trie(ctx, &all_exports, gtk_ml_delete_value);
                    all_exports = new;
                    if (gtk_ml_equal(ext, exp)) {
                        result[i].category &= ~GTKML_I_EXTERN;
                        if (result[i].category == GTKML_I_GENERIC) {
                            result[i].data = result[l].data;
                        } else {
                            *err = gtk_ml_error(ctx, "category-error", GTKML_ERR_CATEGORY_ERROR, 0, 0, 0, 0);
                            return NULL;
                        }
                        found = 1;
                        break;
                    }
                }
            }

            if (found) {
                gtk_ml_del_array_trie(ctx, &all_exports, gtk_ml_delete_value);
            } else {
                GtkMl_SObj exports = gtk_ml_new_array(ctx, NULL);
                exports->value.s_array.array = all_exports;
                GtkMl_SObj error = gtk_ml_error(ctx, "linkage-error", GTKML_ERR_LINKAGE_ERROR, 0, 0, 0, 2,
                    gtk_ml_new_keyword(ctx, NULL, 0, "linkage-name", strlen("linkage-name")), ext,
                    gtk_ml_new_keyword(ctx, NULL, 0, "all-exports", strlen("all-exports")), exports);
                *err = error;
                return NULL;
            }
        }
    }

    if (!complete) {
        char *start = malloc(strlen("_start") + 1);
        strcpy(start, "_start");
        out->start = start;
        out->text = result;
        out->n_text = n;
        out->data = data;
        out->n_data = n_data;
        out->statics = statics;
        out->n_static = n_static;
    } else {
        switch (stage) {
        case GTKML_STAGE_INTR: {
            for (size_t i = 0; i < b->len_bb; i++) {
                free(b->basic_blocks[i]->text);
                free(b->basic_blocks[i]);
            }
            memset(b->basic_blocks, 0, sizeof(GtkMl_BasicBlock *) * b->len_bb);
            b->len_bb = 0;
            b->len_static = 1;
            b->len_data = 1;

            char *start = malloc(strlen("_start") + 1);
            strcpy(start, "_start");
            out->start = start;
            out->text = result;
            out->n_text = n;
            out->data = data;
            out->n_data = n_data;
            out->statics = statics;
            out->n_static = n_static;
        } break;
        case GTKML_STAGE_MACRO: {
            for (size_t i = 0; i < b->len_bb; i++) {
                free(b->basic_blocks[i]->text);
                free(b->basic_blocks[i]);
            }
            memset(b->basic_blocks, 0, sizeof(GtkMl_BasicBlock *) * b->len_bb);
            b->len_bb = 0;
            b->len_static = 1;
            b->len_data = 1;

            char *start = malloc(strlen("_start") + 1);
            strcpy(start, "_start");
            out->start = start;
            out->text = result;
            out->n_text = n;
            out->data = data;
            out->n_data = n_data;
            out->statics = statics;
            out->n_static = n_static;
        } break;
        case GTKML_STAGE_RUNTIME: {
            gtk_ml_del_context(b->macro_ctx);
            gtk_ml_del_context(b->intr_ctx);

            for (size_t i = 0; i < b->len_bb; i++) {
                free(b->basic_blocks[i]->text);
                free(b->basic_blocks[i]);
            }
            for (size_t i = 0; i < b->len_builder; i++) {
                free((void *) b->builders[i].name);
            }
            free(b->builders);
            free(b->base);
            free(b->data);
            free(b->statics);
            free(b->basic_blocks);
            free(b);
            ctx->gc->builder = NULL;

            char *start = malloc(strlen("_start") + 1);
            strcpy(start, "_start");
            out->start = start;
            out->text = result;
            out->n_text = n;
            out->data = data;
            out->n_data = n_data;
            out->statics = statics;
            out->n_static = n_static;
        } break;
        }
    }

    return out;
}

GtkMl_Program *gtk_ml_build_intr_apply(GtkMl_Context *ctx, GtkMl_SObj *err, GtkMl_Builder *b) {
    return build(ctx, err, b, GTKML_STAGE_INTR, 0);
}

GtkMl_Program *gtk_ml_build_intrinsics(GtkMl_Context *ctx, GtkMl_SObj *err, GtkMl_Builder *b) {
    return build(ctx, err, b, GTKML_STAGE_INTR, 1);
}

GtkMl_Program *gtk_ml_build_macros(GtkMl_Context *ctx, GtkMl_SObj *err, GtkMl_Builder *b) {
    return build(ctx, err, b, GTKML_STAGE_MACRO, 1);
}

GtkMl_Program *gtk_ml_build(GtkMl_Context *ctx, GtkMl_SObj *err, GtkMl_Builder *b) {
    return build(ctx, err, b, GTKML_STAGE_RUNTIME, 1);
}
