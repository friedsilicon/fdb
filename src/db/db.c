#define _POSIX_C_SOURCE 200809L

#include "fdb_private.h"
#include "fdb_hexdump.h"
#include "fdb_log.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define MAX_KEY_SIZE    64
#define MAX_DATA_SIZE   1024
#define FDB_DB_ID_MIN   1
#define FDB_DB_ID_INVALID (FDB_DB_ID_MIN - 1)

static int fdb_id = FDB_DB_ID_MIN;

static bool
join_tx(ftx tx, fdb db)
{
    return true;
}

static bool
prepare_tx(ftx tx, fdb db)
{
    return true;
}

static bool
commit_tx(ftx tx, fdb db)
{
    return true;
}

static bool
abort_tx(ftx tx, fdb db)
{
    return true;
}

static bool
finish_tx(ftx tx, fdb db)
{
    return true;
}

static void*
fnode_get_key_priv(struct node_* node)
{
    return (void *) node->content;
}

static void*
fnode_get_data_priv(struct node_* node)
{
    return (void *) node->content + node->header.key_size;
}

struct fdb_traverse_functor {
    traverse_cb callback;
    void*       user_data;
};

static bool 
fdb_traverse_callback(const void* key, void* data, void* user_data)
{
    struct fdb_traverse_functor* functor; 
    assert(user_data);

    functor = (struct fdb_traverse_functor *) user_data;
    assert(functor);
    return functor->callback((fnode) data, functor->user_data);
}

static fdb
fdb_alloc()
{
    return calloc(1, sizeof(fdb_t));
}

static bool
fdb_init_db(fdb db)
{
    assert(db);
    switch (db->type) {
        case FDB_SINGLE_INDEX_DB:
            db->idx_type = DB_SINGLE_INDEX;
            break;
        case FDB_MULTI_INDEX_DB:
            db->idx_type = DB_MULTI_INDEX;
            break;
        default:
            return false;
    }

    return true;
}

static void
fdb_free(fdb db)
{
    if (db) {
        free(db);
        db = NULL;
    }
}

static void
fdb_set_id(fdb db, int id)
{
    assert(db);
    db->id = id;
}

// TODO: ensure uniqueness of the db name, i.e assign dbid using name
static void
fdb_set_name(fdb db, const char* name)
{
    assert(db);

    // TODO: truncation possible
    snprintf(db->name, FDB_MAX_DB_NAME_LEN, "%s", name);
}

static inline void
fdb_log_content(const char* prefix, const void* content, size_t size)
{
    FDB_DEBUG("%s [%lu, %p]", prefix, size, content);
    if (fdb_is_debug_enabled()) {
        hex_dump(prefix, (void *) content, size);
    }
}

void
fdb_log_node_header(struct node_header_* h)
{
   FDB_DEBUG("header:");
   FDB_DEBUG("\tnode_size:%" PRIu32, h->node_size);
   FDB_DEBUG("\tkey_size:%" PRIu32, h->key_size);
   FDB_DEBUG("\tdata_size:%" PRIu32, h->data_size);
   FDB_DEBUG("\tkey_type:%" PRIu16, h->key_type);
   FDB_DEBUG("\tdata_type:%" PRIu16, h->data_type);
}

void
fdb_log_node(struct node_* n)
{
    if (n) {
        FDB_DEBUG("node_p=%p", n);
        fdb_log_node_header(&n->header);
        fdb_log_content("key", fnode_get_key(n), n->header.key_size);
        fdb_log_content("data", fnode_get_data(n), n->header.data_size);
    }
}

#define start_add(ptr1, type1, member1) ((type1 *)((char *)(ptr1) - offsetof(type1, member1)))

static struct node_header_*
fdb_node_get_header_from_key(const void* p)
{
    /* 
    NODE = | node_header_ | key | data |
    
    The header is just above the key, and 
    the key was added as a pointer, and is not
    copied by the underlying implementation. Hence, it 
    ok to do backwards lookup for the pointer 
    */

    /* TODO: write a rbtree implementation that doesn't require 
    key length to be computed this way for comparison aka supported arbitrary keys */

    assert(p);
    struct node_header_* ret = NULL;
    fnode n = (fnode) start_add(p, struct node_, content);
    return &n->header;
}

static bool
fdb_node_header_is_valid(const struct node_header_* h)
{
    if (!h) {
        return false;
    }

    if (h->key_size == 0 || h->key_size >= MAX_KEY_SIZE) {
        return false;
    }
    if (h->data_size > MAX_DATA_SIZE) {
        return false;
    }
    if (h->node_size < sizeof(struct node_header_) + h->key_size + h->data_size) {
        return false;
    }
    if (h->node_size > sizeof(struct node_header_) + MAX_KEY_SIZE + MAX_DATA_SIZE) {
        return false;
    }

    return true;
}

static bool
fdb_key_is_node_backed(const void* key, struct node_header_** out_header)
{
    struct node_header_* h;

    if (!key) {
        return false;
    }

    h = fdb_node_get_header_from_key(key);
    if (!fdb_node_header_is_valid(h)) {
        return false;
    }

    if (out_header) {
        *out_header = h;
    }
    return true;
}

static int
key_cmp(const void* k1, const void* k2)
{
    if (k1 == k2) {
        FDB_DEBUG("Same keys");
        return 0;
    }

    struct node_header_* h1 = NULL;
    struct node_header_* h2 = NULL;
    bool k1_node = fdb_key_is_node_backed(k1, &h1);
    bool k2_node = fdb_key_is_node_backed(k2, &h2);

    size_t len1 = k1_node ? h1->key_size : strlen((const char*) k1) + 1;
    size_t len2 = k2_node ? h2->key_size : strlen((const char*) k2) + 1;
    size_t cmp_len = (len1 < len2) ? len1 : len2;

    int ret = memcmp(k1, k2, cmp_len);
    if (ret != 0) {
        return ret;
    }

    if (len1 < len2) {
        return -1;
    }
    if (len1 > len2) {
        return 1;
    }

    return 0;
}

static void
fdb_node_free(fnode node)
{
    if (node) {
        fdb_log_node(node);
        free(node);
    }
}

static void
key_val_free(void* key, void* datum)
{
    /* just free the data, since it contains the key */
    fdb_node_free((fnode) datum);
}

static struct node_*
fdb_make_node(size_t keysize, size_t datasize)
{
    struct node_* node = NULL;
    uint32_t total_size = 0;

    assert(keysize > 0); /* data can be NULL */
    assert(keysize < MAX_KEY_SIZE);
    assert(datasize < MAX_DATA_SIZE);

    total_size = keysize + datasize + sizeof(struct node_header_);
    FDB_DEBUG("Allocating node of size: %u", total_size);

    node = calloc(1, total_size);
    if (node) {
        node->header.node_size = total_size;
        node->header.key_size  = keysize;
        node->header.data_size = datasize;
    }

    return node;
}

static void
fdb_node_set_key(struct node_* node, const void* key)
{
    assert(node);
    assert(key);
    memmove(fnode_get_key_priv(node), key, node->header.key_size);
}

static void
fdb_node_set_data(struct node_* node, data data)
{
    assert(node);
    assert(data);
    memmove(fnode_get_data_priv(node), data, node->header.data_size);
}

static void
fdb_node_unset_data(struct node_* node)
{
    memset(fnode_get_data_priv(node), 0, node->header.data_size);
}

static fdb
fdb_new(int id, const char* name, fdb_type_t type)
{
    fdb db;

    db = fdb_alloc();
    if (!db) {
        return NULL;
    }

    fdb_set_id(db, id);
    fdb_set_name(db, name);
    db->dstore = rb_dict_new(key_cmp);
    db->type = type;

    db->join = join_tx;
    db->prepare = prepare_tx;
    db->commit = commit_tx;
    db->abort = abort_tx;
    db->finish = finish_tx;

    if (!fdb_init_db(db)) {
        fdb_free(db);
        return NULL;
    }

    return db;
}

key
fnode_get_key(struct node_* node)
{
    if (!node) {
        return NULL;
    }

    return fnode_get_key_priv(node);
}

size_t fnode_get_keysize(fnode node)
{
    if (!node) {
        return 0;
    }

    return node->header.key_size;
}

data
fnode_get_data(struct node_* node)
{
    if (!node) {
        return NULL;
    }

    return fnode_get_data_priv(node);
}

size_t fnode_get_datasize(fnode node)
{
    if (!node) {
        return 0;
    }

    return node->header.data_size;
}

bool
is_fdb_handle_valid(fdb db)
{
    if (!db) {
        return false;
    }

    if (!db->join || !db->prepare || !db->commit || !db->abort || !db->finish) {
        return false;
    }

    return true;
}

fdb
fdb_init(const char* name)
{
    return fdb_init_advanced(name, FDB_SINGLE_INDEX_DB);
}

fdb
fdb_init_advanced(const char* name, fdb_type_t type)
{
    fdb db = NULL;

    if (!name) {
        return NULL;
    }

    db = fdb_new(fdb_id, name, type);
    if (!db) {
        return NULL;
    }

    fdb_id++; /* increment global counter */
    // TODO: thread safety

    return db;
}

void
fdb_deinit(fdb db)
{
    if (db && db->dstore && (db->id != FDB_DB_ID_INVALID)) {
        dict_free(db->dstore, key_val_free);
        db->dstore = NULL;
        db->id = FDB_DB_ID_INVALID;     // to make deinit re-entrant
    }
    fdb_free(db);
}

bool
fdb_insert(fdb db, key key, size_t keysize, data data, size_t datasize)
{
    struct node_* node = NULL;
    dict_insert_result result;
    if (!db || !key || !data || (keysize == 0) || (datasize == 0)) {
        return false;
    }

    node = fdb_make_node(keysize, datasize);
    if (!node) {
        return false;
    }

    fdb_node_set_key(node, key);
    fdb_node_set_data(node, data);

    result = dict_insert(db->dstore, fnode_get_key_priv(node));
    if (result.inserted) {
        *result.datum_ptr = node;
    }

    return result.inserted;
}

bool
fdb_update(fdb db, key key, size_t keysize, data data, size_t datasize)
{
    if (!db || !key || !data || (keysize == 0) || (datasize == 0)) {
        return false;
    }

    fnode old_node = fdb_find(db, key, keysize);
    if (!old_node) {
        return false;
    }

    if (datasize == old_node->header.data_size) {
        fdb_node_unset_data(old_node);
        fdb_node_set_data(old_node, data);
        return true;
    }

    /* Data size changed: allocate a correctly-sized node, swap it in. */
    struct node_* new_node = fdb_make_node(keysize, datasize);
    if (!new_node) {
        return false;
    }
    fdb_node_set_key(new_node, key);
    fdb_node_set_data(new_node, data);

    dict_remove_result rem = dict_remove(db->dstore, key);
    if (!rem.removed) {
        fdb_node_free(new_node);
        return false;
    }

    dict_insert_result ins = dict_insert(db->dstore, fnode_get_key_priv(new_node));
    if (!ins.inserted) {
        fdb_node_free(new_node);
        fdb_node_free((fnode) rem.datum);
        return false;
    }
    *ins.datum_ptr = new_node;
    fdb_node_free((fnode) rem.datum);
    return true;
}

bool
fdb_exists(fdb db, key key, size_t keysize)
{
    return (fdb_find(db, key, keysize) != NULL);
}

bool
fdb_remove(fdb db, key key, size_t keysize)
{
    fnode        node = NULL;
    bool         ret = false;
    dict_remove_result result;

    if (!db || !key || (keysize == 0)) {
        FDB_ERROR("Invalid Arguments");
        return false;
    }

    result = dict_remove(db->dstore, key);
    if (!result.removed) {
        return false;
    }

    fdb_node_free((fnode) result.datum);
    return true;
}

fnode
fdb_find(fdb db, key key, size_t keysize)
{
    if (!db || !key || (keysize == 0)) {
        FDB_ERROR("Invalid Arguments");
        return NULL;
    }

    fnode* ret = NULL;

    FDB_INFO("key: %s, size=%lu", (char *) key, keysize);
    ret = (fnode *) dict_search(db->dstore, key);
    if (ret) {
        hex_dump("data", *ret, 32);
        return *(ret);
    } else {
        return NULL;
    }
}

size_t fdb_traverse(fdb db, traverse_cb callback, void* user_data)
{
    assert(db);
    assert(callback);

    size_t ret;
    struct fdb_traverse_functor functor;
    functor.callback = callback;
    functor.user_data = user_data;

    ret = dict_traverse(db->dstore, &fdb_traverse_callback, &functor);
    return ret;
}

fiter
fdb_iterate(fdb db)
{
    fiter iter = NULL;
    assert(db);

    iter = (fiter) calloc(1, sizeof(struct iter_));
    if (!iter) {
        FDB_ERROR("Out of memory!");
        return NULL;
    }

    iter->it = dict_itor_new(db->dstore);
    if (!iter->it) {
        FDB_ERROR("Cannot create iterator");
        free(iter);
        return NULL;
    }

    if (!dict_itor_first(iter->it)) {
        FDB_ERROR("Cannot create iterator");
        dict_itor_free(iter->it);
        free(iter);
        return NULL;
    }

    if (!dict_itor_valid(iter->it)) {
        FDB_ERROR("No elements in the iterator");
        dict_itor_free(iter->it);
        free(iter);
        return NULL;
    }

    iter->next = (fnode) *dict_itor_datum(iter->it);
    return iter;
}

void
fiter_free(fiter iter)
{
    if (!iter) {
        return;
    }
    if (iter->it) {
        dict_itor_free(iter->it);
    }
    free(iter);
}

bool
fiter_hasnext(fiter iter)
{
    if (!iter || !iter->it || !iter->next) {
        return false;
    }

    return true;
}

fnode
fiter_next(fiter iter)
{
    fnode current;
    if (!iter || !iter->it || !dict_itor_valid(iter->it)) {
        return NULL;
    }

    current = iter->next;
    iter->next = NULL;
    if (dict_itor_next(iter->it)) {
        iter->next = (fnode) *dict_itor_datum(iter->it);
    }

    return current;
}

void 
fdb_log_db_state(const fdb db)
{
    FDB_DEBUG("id=%d, type=%d, idx_type=%d, name=%s", db->id, db->type, db->idx_type, db->name);
}
