#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define GTKML_INCLUDE_INTERNAL
#include "gtk-ml.h"
#include "gtk-ml-internal.h"

#define GTKML_H_BITS 5
#define GTKML_H_SIZE (1 << GTKML_H_BITS)
#define GTKML_H_MASK (GTKML_H_SIZE - 1)

typedef enum GtkMl_HashTrieNodeKind {
    GTKML_HT_LEAF,
    GTKML_HT_BRANCH,
} GtkMl_HashTrieNodeKind;

typedef struct GtkMl_HLeaf {
    GtkMl_TaggedValue key;
    GtkMl_TaggedValue value;
} GtkMl_HLeaf;

typedef struct GtkMl_HBranch {
    GtkMl_HashTrieNode **nodes;
} GtkMl_HBranch;

typedef union GtkMl_HUnion {
    GtkMl_HLeaf h_leaf;
    GtkMl_HBranch h_branch;
} GtkMl_HUnion;

struct GtkMl_HashTrieNode {
    int rc;
    GtkMl_HashTrieNodeKind kind;
    GtkMl_HUnion value;
};

GTKML_PRIVATE GtkMl_HashTrieNode *new_leaf(GtkMl_TaggedValue key, GtkMl_TaggedValue value);
GTKML_PRIVATE GtkMl_HashTrieNode *new_branch();
GTKML_PRIVATE GtkMl_HashTrieNode *copy_node(GtkMl_HashTrieNode *node);
GTKML_PRIVATE void del_node(GtkMl_Context *ctx, GtkMl_HashTrieNode *node, void (*deleter)(GtkMl_Context *, GtkMl_TaggedValue));
GTKML_PRIVATE GtkMl_TaggedValue insert(GtkMl_Hasher *hasher, GtkMl_HashTrieNode **out, size_t *inc, GtkMl_HashTrieNode *node, GtkMl_TaggedValue key, GtkMl_TaggedValue value, GtkMl_Hash hash, uint32_t shift);
GTKML_PRIVATE GtkMl_TaggedValue get(GtkMl_Hasher *hasher, GtkMl_HashTrieNode *node, GtkMl_TaggedValue key, GtkMl_Hash hash, uint32_t shift);
GTKML_PRIVATE GtkMl_TaggedValue delete(GtkMl_Hasher *hasher, GtkMl_HashTrieNode **out, size_t *dec, GtkMl_HashTrieNode *node, GtkMl_TaggedValue key, GtkMl_Hash hash, uint32_t shift);
GTKML_PRIVATE GtkMl_VisitResult foreach(GtkMl_HashTrie *ht, GtkMl_HashTrieNode *node, GtkMl_HashTrieFn fn, GtkMl_TaggedValue data);
GTKML_PRIVATE gboolean equal(GtkMl_Hasher *hasher, GtkMl_HashTrieNode *lhs, GtkMl_HashTrieNode *rhs);

void gtk_ml_new_hash_trie(GtkMl_HashTrie *ht, GtkMl_Hasher *hasher) {
    ht->hasher = hasher;
    ht->root = NULL;
    ht->len = 0;
}

void gtk_ml_del_hash_trie(GtkMl_Context *ctx, GtkMl_HashTrie *ht, void (*deleter)(GtkMl_Context *, GtkMl_TaggedValue)) {
    del_node(ctx, ht->root, deleter);
    ht->root = NULL;
    ht->len = 0;
}

void gtk_ml_hash_trie_copy(GtkMl_HashTrie *out, GtkMl_HashTrie *ht) {
    out->hasher = ht->hasher;
    out->root = copy_node(ht->root);
    out->len = ht->len;
}

size_t gtk_ml_hash_trie_len(GtkMl_HashTrie *ht) {
    return ht->len;
}

GTKML_PRIVATE GtkMl_VisitResult fn_concat(GtkMl_HashTrie *ht, GtkMl_TaggedValue key, GtkMl_TaggedValue value, GtkMl_TaggedValue data) {
    (void) ht;

    GtkMl_HashTrie *dest = data.value.userdata;

    GtkMl_HashTrie new;
    gtk_ml_hash_trie_insert(&new, dest, key, value);
    *dest = new;
    
    return GTKML_VISIT_RECURSE;
}

void gtk_ml_hash_trie_concat(GtkMl_HashTrie *out, GtkMl_HashTrie *lhs, GtkMl_HashTrie *rhs) {
    out->hasher = lhs->hasher;
    out->root = copy_node(lhs->root);
    out->len = lhs->len;

    gtk_ml_hash_trie_foreach(rhs, fn_concat, gtk_ml_value_userdata(out));
}

GtkMl_TaggedValue gtk_ml_hash_trie_insert(GtkMl_HashTrie *out, GtkMl_HashTrie *ht, GtkMl_TaggedValue key, GtkMl_TaggedValue value) {
    out->hasher = ht->hasher;
    out->root = NULL;
    out->len = ht->len;

    GtkMl_Hash hash;
    if (!gtk_ml_hash(ht->hasher, &hash, key)) {
        return gtk_ml_value_none();
    }
    return insert(ht->hasher, &out->root, &out->len, ht->root, key, value, hash, 0);
}

GtkMl_TaggedValue gtk_ml_hash_trie_get(GtkMl_HashTrie *ht, GtkMl_TaggedValue key) {
    GtkMl_Hash hash;
    if (!gtk_ml_hash(ht->hasher, &hash, key)) {
        return gtk_ml_value_none();
    }
    return get(ht->hasher, ht->root, key, hash, 0);
}

gboolean gtk_ml_hash_trie_contains(GtkMl_HashTrie *ht, GtkMl_TaggedValue key) {
    GtkMl_Hash hash;
    if (!gtk_ml_hash(ht->hasher, &hash, key)) {
        return 0;
    }
    return gtk_ml_has_value(get(ht->hasher, ht->root, key, hash, 0));
}

GtkMl_TaggedValue gtk_ml_hash_trie_delete(GtkMl_HashTrie *out, GtkMl_HashTrie *ht, GtkMl_TaggedValue key) {
    out->hasher = ht->hasher;
    out->root = NULL;
    out->len = ht->len;

    GtkMl_Hash hash;
    if (!gtk_ml_hash(ht->hasher, &hash, key)) {
        return gtk_ml_value_none();
    }
    return delete(out->hasher, &out->root, &out->len, ht->root, key, hash, 0);
}

void gtk_ml_hash_trie_foreach(GtkMl_HashTrie *ht, GtkMl_HashTrieFn fn, GtkMl_TaggedValue data) {
    foreach(ht, ht->root, fn, data);
}

gboolean gtk_ml_hash_trie_equal(GtkMl_HashTrie *lhs, GtkMl_HashTrie *rhs) {
    if (lhs->len != rhs->len) {
        return 0;
    }

    return equal(lhs->hasher, lhs->root, rhs->root);
}

GtkMl_HashTrieNode *new_leaf(GtkMl_TaggedValue key, GtkMl_TaggedValue value) {
    GtkMl_HashTrieNode *node = malloc(sizeof(GtkMl_HashTrieNode));
    node->rc = 1;
    node->kind = GTKML_HT_LEAF;
    node->value.h_leaf.key = key;
    node->value.h_leaf.value = value;
    return node;
}

GtkMl_HashTrieNode *new_branch() {
    GtkMl_HashTrieNode *node = malloc(sizeof(GtkMl_HashTrieNode));
    node->rc = 1;
    node->kind = GTKML_HT_BRANCH;
    node->value.h_branch.nodes = malloc(sizeof(GtkMl_HashTrieNode *) * GTKML_H_SIZE);
    memset(node->value.h_branch.nodes, 0, sizeof(GtkMl_HashTrieNode *) * GTKML_H_SIZE);
    return node;
}

GtkMl_HashTrieNode *copy_node(GtkMl_HashTrieNode *node) {
    if (!node) {
        return NULL;
    }

    ++node->rc;

    return node;
}

void del_node(GtkMl_Context *ctx, GtkMl_HashTrieNode *node, void (*deleter)(GtkMl_Context *, GtkMl_TaggedValue)) {
    if (!node) {
        return;
    }

    --node->rc;
    if (!node->rc) {
        switch (node->kind) {
        case GTKML_HT_LEAF:
            deleter(ctx, node->value.h_leaf.key);
            deleter(ctx, node->value.h_leaf.value);
            break;
        case GTKML_HT_BRANCH:
            for (size_t i = 0; i < GTKML_H_SIZE; i++) {
                del_node(ctx, node->value.h_branch.nodes[i], deleter);
            }
            free(node->value.h_branch.nodes);
            break;
        }
        free(node);
    }
}

GtkMl_TaggedValue insert(GtkMl_Hasher *hasher, GtkMl_HashTrieNode **out, size_t *inc, GtkMl_HashTrieNode *node, GtkMl_TaggedValue key, GtkMl_TaggedValue value, GtkMl_Hash hash, uint32_t shift) {
    if (!node) {
        ++*inc;
        *out = new_leaf(key, value);
        return gtk_ml_value_none();
    }

    switch (node->kind) {
    case GTKML_HT_LEAF:
        if (hasher->equal(key, node->value.h_leaf.key)) {
            *out = new_leaf(key, value);
            return node->value.h_leaf.value;
        } else {
            *out = new_branch();

            GtkMl_Hash _hash;
            if (!gtk_ml_hash(hasher, &_hash, node->value.h_leaf.key)) {
                return gtk_ml_value_none();
            }
            uint32_t _idx = (_hash >> shift) & GTKML_H_MASK;
            (*out)->value.h_branch.nodes[_idx] = copy_node(node);

            if (hash == _hash) {
                fprintf(stderr, "fatal error: two non-equal keys in a hash map have the same hashes %"GTKML_FMT_64"x, %"GTKML_FMT_64"x\n", key.value.u64, node->value.h_leaf.key.value.u64);
                if (gtk_ml_is_sobject(key)) {
                    (void) gtk_ml_dumpf(NULL, stderr, NULL, key.value.sobj);
                    fprintf(stderr, ", ");
                }
                if (gtk_ml_is_sobject(node->value.h_leaf.key)) {
                    (void) gtk_ml_dumpf(NULL, stderr, NULL, node->value.h_leaf.key.value.sobj);
                }
                fprintf(stderr, "\n");
                exit(1);
            }

            uint32_t idx = (hash >> shift) & GTKML_H_MASK;
            return insert(hasher, &(*out)->value.h_branch.nodes[idx], inc, (*out)->value.h_branch.nodes[idx], key, value, hash, shift + GTKML_H_BITS);
        }
    case GTKML_HT_BRANCH: {
        *out = new_branch();
        for (size_t i = 0; i < GTKML_H_SIZE; i++) {
            (*out)->value.h_branch.nodes[i] = copy_node(node->value.h_branch.nodes[i]);
        }
        uint32_t idx = (hash >> shift) & GTKML_H_MASK;
        return insert(hasher, &(*out)->value.h_branch.nodes[idx], inc, node->value.h_branch.nodes[idx], key, value, hash, shift + GTKML_H_BITS);
    }
    }
}

GtkMl_TaggedValue get(GtkMl_Hasher *hasher, GtkMl_HashTrieNode *node, GtkMl_TaggedValue key, GtkMl_Hash hash, uint32_t shift) {
    if (!node) {
        return gtk_ml_value_none();
    }

    switch (node->kind) {
    case GTKML_HT_LEAF:
        if (hasher->equal(node->value.h_leaf.key, key)) {
            return node->value.h_leaf.value;
        } else {
            return gtk_ml_value_none();
        }
    case GTKML_HT_BRANCH: {
        uint32_t idx = (hash >> shift) & GTKML_H_MASK;
        return get(hasher, node->value.h_branch.nodes[idx], key, hash, shift + GTKML_H_BITS);
    }
    }
}

GtkMl_TaggedValue delete(GtkMl_Hasher *hasher, GtkMl_HashTrieNode **out, size_t *dec, GtkMl_HashTrieNode *node, GtkMl_TaggedValue key, GtkMl_Hash hash, uint32_t shift) {
    if (!node) {
        return gtk_ml_value_none();
    }

    switch (node->kind) {
    case GTKML_HT_LEAF:
        if (hasher->equal(node->value.h_leaf.key, key)) {
            --*dec;
            del_node(NULL, *out, gtk_ml_delete_value);
            *out = NULL;
            return node->value.h_leaf.value;
        } else {
            return gtk_ml_value_none();
        }
    case GTKML_HT_BRANCH: {
        *out = new_branch();
        for (size_t i = 0; i < GTKML_H_SIZE; i++) {
            (*out)->value.h_branch.nodes[i] = copy_node(node->value.h_branch.nodes[i]);
        }
        uint32_t idx = (hash >> shift) & GTKML_H_MASK;
        return delete(hasher, &(*out)->value.h_branch.nodes[idx], dec, node->value.h_branch.nodes[idx], key, hash, shift + GTKML_H_BITS);
    }
    }
}

GtkMl_VisitResult foreach(GtkMl_HashTrie *ht, GtkMl_HashTrieNode *node, GtkMl_HashTrieFn fn, GtkMl_TaggedValue data) {
    if (!node) {
        return GTKML_VISIT_RECURSE;
    }

    switch (node->kind) {
    case GTKML_HT_LEAF:
        return fn(ht, node->value.h_leaf.key, node->value.h_leaf.value, data);
    case GTKML_HT_BRANCH: {
        for (size_t i = 0; i < GTKML_H_SIZE; i++) {
            switch (foreach(ht, node->value.h_branch.nodes[i], fn, data)) {
            case GTKML_VISIT_RECURSE:
                continue;
            case GTKML_VISIT_CONTINUE:
                return GTKML_VISIT_RECURSE;
            case GTKML_VISIT_BREAK:
                return GTKML_VISIT_BREAK;
            }
        }
        return GTKML_VISIT_RECURSE;
    }
    }
}

gboolean equal(GtkMl_Hasher *hasher, GtkMl_HashTrieNode *lhs, GtkMl_HashTrieNode *rhs) {
    if (lhs == rhs) {
        return 1;
    }

    if (lhs->kind != rhs->kind) {
        return 0;
    }

    switch (lhs->kind) {
    case GTKML_HT_LEAF:
        return hasher->equal(lhs->value.h_leaf.key, rhs->value.h_leaf.key) && hasher->equal(lhs->value.h_leaf.value, rhs->value.h_leaf.value);
    case GTKML_HT_BRANCH:
        for (size_t i = 0; i < GTKML_H_SIZE; i++) {
            if (!equal(hasher, lhs->value.h_branch.nodes[i], rhs->value.h_branch.nodes[i])) {
                return 0;
            }
        }
        return 1;
    }
}

#ifdef GTKML_ENABLE_POSIX
/* debug stuff */

// GTKML_PRIVATE GtkMl_HashTrieNode *new_leaf_debug(GtkMl_Context *ctx, GtkMl_TaggedValue key, GtkMl_TaggedValue value);
// GTKML_PRIVATE GtkMl_HashTrieNode *new_branch_debug(GtkMl_Context *ctx);
GTKML_PRIVATE GtkMl_HashTrieNode *copy_node_debug(GtkMl_Context *ctx, GtkMl_HashTrieNode *node);
// GTKML_PRIVATE void del_node_debug(GtkMl_Context *ctx, GtkMl_HashTrieNode *node, void (*deleter)(GtkMl_Context *, GtkMl_TaggedValue));
GTKML_PRIVATE GtkMl_TaggedValue insert_debug(GtkMl_Context *ctx, GtkMl_Hasher *hasher, GtkMl_HashTrieNode **out, size_t *inc, GtkMl_HashTrieNode *node, GtkMl_TaggedValue key, GtkMl_TaggedValue value, GtkMl_Hash hash, uint32_t shift);
GTKML_PRIVATE GtkMl_TaggedValue get_debug(GtkMl_Context *ctx, GtkMl_Hasher *hasher, GtkMl_HashTrieNode *node, GtkMl_TaggedValue key, GtkMl_Hash hash, uint32_t shift);
GTKML_PRIVATE GtkMl_TaggedValue delete_debug(GtkMl_Context *ctx, GtkMl_Hasher *hasher, GtkMl_HashTrieNode **out, size_t *dec, GtkMl_HashTrieNode *node, GtkMl_TaggedValue key, GtkMl_Hash hash, uint32_t shift);
GTKML_PRIVATE GtkMl_VisitResult foreach_debug(GtkMl_Context *ctx, GtkMl_SObj *err, GtkMl_HashTrie *ht, GtkMl_HashTrieNode *node, GtkMl_HashTrieDebugFn fn, GtkMl_TaggedValue data);
GTKML_PRIVATE gboolean equal_debug(GtkMl_Context *ctx, GtkMl_Hasher *hasher, GtkMl_HashTrieNode *lhs, GtkMl_HashTrieNode *rhs);

void gtk_ml_hash_trie_copy_debug(GtkMl_Context *ctx, GtkMl_HashTrie *out, GtkMl_HashTrie *ht) {
    out->hasher = ht->hasher;
    out->root = copy_node_debug(ctx, ht->root);
    out->len = ht->len;
}

size_t gtk_ml_hash_trie_len_debug(GtkMl_Context *ctx, GtkMl_HashTrie *ht) {
    (void) ctx;
    return ht->len;
}

GTKML_PRIVATE GtkMl_VisitResult fn_concat_debug(GtkMl_Context *ctx, GtkMl_SObj *err, GtkMl_HashTrie *ht, GtkMl_TaggedValue key, GtkMl_TaggedValue value, GtkMl_TaggedValue data) {
    (void) ht;
    (void) err;

    GtkMl_HashTrie *dest = data.value.userdata;

    GtkMl_HashTrie new;
    gtk_ml_hash_trie_insert_debug(ctx, &new, dest, key, value);
    *dest = new;
    
    return GTKML_VISIT_RECURSE;
}

gboolean gtk_ml_hash_trie_concat_debug(GtkMl_Context *ctx, GtkMl_SObj *err, GtkMl_HashTrie *out, GtkMl_HashTrie *lhs, GtkMl_HashTrie *rhs) {
    out->hasher = lhs->hasher;
    out->root = copy_node_debug(ctx, lhs->root);
    out->len = lhs->len;

    return gtk_ml_hash_trie_foreach_debug(ctx, err, rhs, fn_concat_debug, gtk_ml_value_userdata(out));
}

GtkMl_TaggedValue gtk_ml_hash_trie_insert_debug(GtkMl_Context *ctx, GtkMl_HashTrie *out, GtkMl_HashTrie *ht, GtkMl_TaggedValue key, GtkMl_TaggedValue value) {
    out->hasher = ht->hasher;
    out->root = NULL;
    out->len = ht->len;

    GtkMl_Hash hash;
    if (!gtk_ml_hash_debug(ctx, ht->hasher, &hash, key)) {
        return gtk_ml_value_none();
    }
    return insert_debug(ctx, ht->hasher, &out->root, &out->len, ht->root, key, value, hash, 0);
}

GtkMl_TaggedValue gtk_ml_hash_trie_get_debug(GtkMl_Context *ctx, GtkMl_HashTrie *ht, GtkMl_TaggedValue key) {
    GtkMl_Hash hash;
    if (!gtk_ml_hash_debug(ctx, ht->hasher, &hash, key)) {
        return gtk_ml_value_none();
    }
    return get_debug(ctx, ht->hasher, ht->root, key, hash, 0);
}

gboolean gtk_ml_hash_trie_contains_debug(GtkMl_Context *ctx, GtkMl_HashTrie *ht, GtkMl_TaggedValue key) {
    GtkMl_Hash hash;
    if (!gtk_ml_hash_debug(ctx, ht->hasher, &hash, key)) {
        return 0;
    }
    return gtk_ml_has_value(get_debug(ctx, ht->hasher, ht->root, key, hash, 0));
}

GtkMl_TaggedValue gtk_ml_hash_trie_delete_debug(GtkMl_Context *ctx, GtkMl_HashTrie *out, GtkMl_HashTrie *ht, GtkMl_TaggedValue key) {
    out->hasher = ht->hasher;
    out->root = NULL;
    out->len = ht->len;

    GtkMl_Hash hash;
    if (!gtk_ml_hash_debug(ctx, ht->hasher, &hash, key)) {
        return gtk_ml_value_none();
    }
    return delete_debug(ctx, out->hasher, &out->root, &out->len, ht->root, key, hash, 0);
}

gboolean gtk_ml_hash_trie_foreach_debug(GtkMl_Context *ctx, GtkMl_SObj *err, GtkMl_HashTrie *ht, GtkMl_HashTrieDebugFn fn, GtkMl_TaggedValue data) {
    *err = NULL;
    foreach_debug(ctx, err, ht, ht->root, fn, data);
    return *err == NULL;
}

gboolean gtk_ml_hash_trie_equal_debug(GtkMl_Context *ctx, GtkMl_HashTrie *lhs, GtkMl_HashTrie *rhs) {
    if (lhs->len != rhs->len) {
        return 0;
    }

    return equal_debug(ctx, lhs->hasher, lhs->root, rhs->root);
}

// GtkMl_HashTrieNode *new_leaf_debug(GtkMl_Context *ctx, GtkMl_TaggedValue key, GtkMl_TaggedValue value) {
//     (void) ctx;
//     (void) key;
//     (void) value;
//     fprintf(stderr, "warning: new_leaf is currently unavailable in debug mode\n");
//     return NULL;
// }
// 
// GtkMl_HashTrieNode *new_branch_debug(GtkMl_Context *ctx) {
//     (void) ctx;
//     fprintf(stderr, "warning: new_branch is currently unavailable in debug mode\n");
//     return NULL;
// }

GtkMl_HashTrieNode *copy_node_debug(GtkMl_Context *ctx, GtkMl_HashTrieNode *node) {
    (void) ctx;
    (void) node;
    fprintf(stderr, "warning: copy_node is currently unavailable in debug mode\n");
    return NULL;
}

// void del_node_debug(GtkMl_Context *ctx, GtkMl_HashTrieNode *node, void (*deleter)(GtkMl_Context *, GtkMl_TaggedValue)) {
//     (void) ctx;
//     (void) ctx;
//     (void) node;
//     (void) deleter;
//     fprintf(stderr, "warning: del_node is currently unavailable in debug mode\n");
// }

GtkMl_TaggedValue insert_debug(GtkMl_Context *ctx, GtkMl_Hasher *hasher, GtkMl_HashTrieNode **out, size_t *inc, GtkMl_HashTrieNode *node, GtkMl_TaggedValue key, GtkMl_TaggedValue value, GtkMl_Hash hash, uint32_t shift) {
    (void) ctx;
    (void) hasher;
    (void) out;
    (void) inc;
    (void) node;
    (void) key;
    (void) value;
    (void) hash;
    (void) shift;
    fprintf(stderr, "warning: insert is currently unavailable in debug mode\n");
    return gtk_ml_value_none();
}

GtkMl_TaggedValue get_debug(GtkMl_Context *ctx, GtkMl_Hasher *hasher, GtkMl_HashTrieNode *node, GtkMl_TaggedValue key, GtkMl_Hash hash, uint32_t shift) {
    (void) ctx;
    (void) hasher;
    (void) node;
    (void) key;
    (void) hash;
    (void) shift;
    fprintf(stderr, "warning: get is currently unavailable in debug mode\n");
    return gtk_ml_value_none();
}

GtkMl_TaggedValue delete_debug(GtkMl_Context *ctx, GtkMl_Hasher *hasher, GtkMl_HashTrieNode **out, size_t *dec, GtkMl_HashTrieNode *node, GtkMl_TaggedValue key, GtkMl_Hash hash, uint32_t shift) {
    (void) ctx;
    (void) hasher;
    (void) out;
    (void) dec;
    (void) node;
    (void) key;
    (void) hash;
    (void) shift;
    fprintf(stderr, "warning: get is currently unavailable in debug mode\n");
    return gtk_ml_value_none();
}

GtkMl_VisitResult foreach_debug(GtkMl_Context *ctx, GtkMl_SObj *err, GtkMl_HashTrie *ht, GtkMl_HashTrieNode *node, GtkMl_HashTrieDebugFn fn, GtkMl_TaggedValue data) {
    if (!node) {
        return GTKML_VISIT_RECURSE;
    }

    GtkMl_HashTrieNodeKind kind = (GtkMl_HashTrieNodeKind) gtk_ml_dbg_read_u32(ctx, err, &node->kind);
    if (*err) {
        return GTKML_VISIT_BREAK;
    }
    switch (kind) {
    case GTKML_HT_LEAF: {
        GtkMl_TaggedValue key = gtk_ml_dbg_read_value(ctx, err, &node->value.h_leaf.key);
        if (*err) {
            return GTKML_VISIT_BREAK;
        }
        GtkMl_TaggedValue value = gtk_ml_dbg_read_value(ctx, err, &node->value.h_leaf.value);
        if (*err) {
            return GTKML_VISIT_BREAK;
        }
        return fn(ctx, err, ht, key, value, data);
    }
    case GTKML_HT_BRANCH: {
        for (size_t i = 0; i < GTKML_H_SIZE; i++) {
            GtkMl_HashTrieNode **nodes = gtk_ml_dbg_read_ptr(ctx, err, &node->value.h_branch.nodes);
            if (*err) {
                return GTKML_VISIT_BREAK;
            }
            GtkMl_HashTrieNode *next = gtk_ml_dbg_read_ptr(ctx, err, &nodes[i]);
            if (*err) {
                return GTKML_VISIT_BREAK;
            }
            switch (foreach_debug(ctx, err, ht, next, fn, data)) {
            case GTKML_VISIT_RECURSE:
                continue;
            case GTKML_VISIT_CONTINUE:
                return GTKML_VISIT_RECURSE;
            case GTKML_VISIT_BREAK:
                return GTKML_VISIT_BREAK;
            }
        }
        return GTKML_VISIT_RECURSE;
    }
    }
}

gboolean equal_debug(GtkMl_Context *ctx, GtkMl_Hasher *hasher, GtkMl_HashTrieNode *lhs, GtkMl_HashTrieNode *rhs) {
    if (lhs == rhs) {
        return 1;
    }

    (void) ctx;
    (void) hasher;
    fprintf(stderr, "warning: equal is currently unavailable in debug mode\n");
    return 0;
}
#endif /* GTKML_ENABLE_POSIX */
