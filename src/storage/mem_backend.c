/*
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 *
 * In-memory storage backend — wraps libdict's red-black tree.
 *
 * Internally every entry is stored as a single heap allocation (a "node")
 * with the layout:
 *
 *   [ node_header_ | key bytes | value bytes ]
 *
 * The dict key is a pointer to the first byte of "key bytes", which lets
 * the key comparator (key_cmp) recover the key length via the header that
 * sits immediately before it in memory.
 */

#include "mem_backend.h"
#include "fdb_log.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dict.h"
#include "rb_tree.h"

/* ---------------------------------------------------------------------- */
/* Internal node layout                                                   */
/* ---------------------------------------------------------------------- */

#define MEM_MAX_KEY_SIZE  64
#define MEM_MAX_VAL_SIZE  1024

struct mem_node_header_ {
    uint32_t node_size;
    uint32_t key_size;
    uint32_t val_size;
};

struct mem_node_ {
    struct mem_node_header_ header;
    uint8_t                 content[0]; /* [key_size bytes][val_size bytes] */
};

#define start_add(ptr, type, member) \
    ((type*)((char*)(ptr) - offsetof(type, member)))

static inline void*
node_key_ptr(struct mem_node_* n)
{
    return (void*)n->content;
}

static inline void*
node_val_ptr(struct mem_node_* n)
{
    return (void*)(n->content + n->header.key_size);
}

/* ---------------------------------------------------------------------- */
/* Key comparator                                                         */
/* ---------------------------------------------------------------------- */

static int
key_cmp(const void* k1, const void* k2)
{
    if (k1 == k2) return 0;

    /* Each key pointer may or may not be backed by a mem_node_ header.
     * We detect this by checking whether the header looks valid.         */
    const struct mem_node_header_* h1 = NULL;
    const struct mem_node_header_* h2 = NULL;

    /* Back-cast: the key pointer is content[0], header is just before.  */
    const struct mem_node_* n1 =
        start_add((void*)k1, struct mem_node_, content);
    const struct mem_node_* n2 =
        start_add((void*)k2, struct mem_node_, content);

    /* A node header is valid when sizes are in range and consistent.     */
    bool k1_node = (n1->header.key_size > 0 &&
                    n1->header.key_size < MEM_MAX_KEY_SIZE &&
                    n1->header.val_size <= MEM_MAX_VAL_SIZE &&
                    n1->header.node_size ==
                        sizeof(struct mem_node_header_) +
                        n1->header.key_size + n1->header.val_size);
    bool k2_node = (n2->header.key_size > 0 &&
                    n2->header.key_size < MEM_MAX_KEY_SIZE &&
                    n2->header.val_size <= MEM_MAX_VAL_SIZE &&
                    n2->header.node_size ==
                        sizeof(struct mem_node_header_) +
                        n2->header.key_size + n2->header.val_size);

    size_t len1 = k1_node ? n1->header.key_size : strlen((const char*)k1) + 1;
    size_t len2 = k2_node ? n2->header.key_size : strlen((const char*)k2) + 1;
    size_t cmp_len = len1 < len2 ? len1 : len2;

    int ret = memcmp(k1, k2, cmp_len);
    if (ret != 0) return ret;
    if (len1 < len2) return -1;
    if (len1 > len2) return  1;
    return 0;
}

/* ---------------------------------------------------------------------- */
/* Node helpers                                                           */
/* ---------------------------------------------------------------------- */

static struct mem_node_*
node_alloc(const void* key, size_t ksz, const void* val, size_t vsz)
{
    assert(ksz > 0 && ksz < MEM_MAX_KEY_SIZE);
    assert(vsz <= MEM_MAX_VAL_SIZE);

    uint32_t total = (uint32_t)(sizeof(struct mem_node_header_) + ksz + vsz);
    struct mem_node_* n = calloc(1, total);
    if (!n) return NULL;

    n->header.node_size = total;
    n->header.key_size  = (uint32_t)ksz;
    n->header.val_size  = (uint32_t)vsz;
    memcpy(node_key_ptr(n), key, ksz);
    memcpy(node_val_ptr(n), val, vsz);
    return n;
}

static void
node_free_kv(void* key, void* datum)
{
    (void)key;
    free(datum); /* datum is the mem_node_; key points inside it */
}

static fdb_node_t
node_to_view(struct mem_node_* n)
{
    return (fdb_node_t){
        .key       = node_key_ptr(n),
        .key_size  = n->header.key_size,
        .data      = node_val_ptr(n),
        .data_size = n->header.val_size,
    };
}

/* ---------------------------------------------------------------------- */
/* Backend context                                                        */
/* ---------------------------------------------------------------------- */

typedef struct {
    dict* store;
} mem_ctx_t;

/* ---------------------------------------------------------------------- */
/* Cursor                                                                 */
/* ---------------------------------------------------------------------- */

typedef struct {
    dict_itor* it;
    fdb_node_t current;
} mem_cursor_t;

/* ---------------------------------------------------------------------- */
/* Backend ops implementation                                             */
/* ---------------------------------------------------------------------- */

static bool
mem_open(void* ctx)
{
    mem_ctx_t* c = ctx;
    c->store = rb_dict_new(key_cmp);
    return c->store != NULL;
}

static void
mem_close(void* ctx)
{
    mem_ctx_t* c = ctx;
    if (c->store) {
        dict_free(c->store, node_free_kv);
        c->store = NULL;
    }
}

static bool
mem_insert(void* ctx, const void* key, size_t ksz,
                       const void* val, size_t vsz)
{
    mem_ctx_t* c = ctx;
    struct mem_node_* n = node_alloc(key, ksz, val, vsz);
    if (!n) return false;

    dict_insert_result r = dict_insert(c->store, node_key_ptr(n));
    if (!r.inserted) {
        free(n);
        return false;
    }
    *r.datum_ptr = n;
    return true;
}

static bool
mem_update(void* ctx, const void* key, size_t ksz,
                       const void* val, size_t vsz)
{
    mem_ctx_t* c = ctx;

    /* Allocate the replacement node first so we can roll back on failure.*/
    struct mem_node_* new_node = node_alloc(key, ksz, val, vsz);
    if (!new_node) return false;

    dict_remove_result rem = dict_remove(c->store, key);
    if (!rem.removed) {
        free(new_node);
        return false;
    }

    dict_insert_result ins = dict_insert(c->store, node_key_ptr(new_node));
    if (!ins.inserted) {
        free(new_node);
        free(rem.datum);
        return false;
    }
    *ins.datum_ptr = new_node;
    free(rem.datum);
    return true;
}

static bool
mem_remove(void* ctx, const void* key, size_t ksz)
{
    (void)ksz;
    mem_ctx_t* c = ctx;
    dict_remove_result r = dict_remove(c->store, key);
    if (!r.removed) return false;
    free(r.datum);
    return true;
}

static fdb_node_t
mem_find(void* ctx, const void* key, size_t ksz)
{
    (void)ksz;
    mem_ctx_t* c = ctx;
    void** slot = dict_search(c->store, key);
    if (!slot) return FDB_NODE_INVALID;
    return node_to_view((struct mem_node_*)*slot);
}

static bool
mem_traverse_visit(const void* key, void* datum, void* userdata)
{
    (void)key;
    struct {
        fdb_storage_visit_fn fn;
        void* ud;
    }* wrap = userdata;
    return wrap->fn(node_to_view((struct mem_node_*)datum), wrap->ud);
}

static size_t
mem_traverse(void* ctx, fdb_storage_visit_fn visit, void* userdata)
{
    mem_ctx_t* c = ctx;
    struct { fdb_storage_visit_fn fn; void* ud; } wrap = { visit, userdata };
    return dict_traverse(c->store, mem_traverse_visit, &wrap);
}

static bool
mem_cursor_open(void* ctx, void* cursor_buf)
{
    mem_ctx_t*   c   = ctx;
    mem_cursor_t* cur = cursor_buf;
    cur->it      = dict_itor_new(c->store);
    cur->current = FDB_NODE_INVALID;
    return cur->it != NULL;
}

static bool
mem_cursor_first(void* cursor_buf)
{
    mem_cursor_t* cur = cursor_buf;
    return dict_itor_first(cur->it);
}

static bool
mem_cursor_valid(void* cursor_buf)
{
    mem_cursor_t* cur = cursor_buf;
    return dict_itor_valid(cur->it);
}

static bool
mem_cursor_next(void* cursor_buf)
{
    mem_cursor_t* cur = cursor_buf;
    return dict_itor_next(cur->it);
}

static fdb_node_t
mem_cursor_get(void* cursor_buf)
{
    mem_cursor_t* cur = cursor_buf;
    if (!dict_itor_valid(cur->it)) return FDB_NODE_INVALID;
    void** slot = dict_itor_datum(cur->it);
    return node_to_view((struct mem_node_*)*slot);
}

static void
mem_cursor_close(void* cursor_buf)
{
    mem_cursor_t* cur = cursor_buf;
    if (cur->it) {
        dict_itor_free(cur->it);
        cur->it = NULL;
    }
}

/* ---------------------------------------------------------------------- */
/* Public ops table                                                       */
/* ---------------------------------------------------------------------- */

static const fdb_storage_ops_t k_mem_ops = {
    .ctx_size        = sizeof(mem_ctx_t),
    .cursor_buf_size = sizeof(mem_cursor_t),
    .open            = mem_open,
    .close           = mem_close,
    .insert          = mem_insert,
    .update          = mem_update,
    .remove          = mem_remove,
    .find            = mem_find,
    .traverse        = mem_traverse,
    .cursor_open     = mem_cursor_open,
    .cursor_first    = mem_cursor_first,
    .cursor_valid    = mem_cursor_valid,
    .cursor_next     = mem_cursor_next,
    .cursor_get      = mem_cursor_get,
    .cursor_close    = mem_cursor_close,
};

const fdb_storage_ops_t*
fdb_mem_backend(void)
{
    return &k_mem_ops;
}
