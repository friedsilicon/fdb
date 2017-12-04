#define _POSIX_C_SOURCE 200809L

#include "fdb_private.h"
#include "fdb_hexdump.h"
#include "fdb_log.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MAX_KEY_SIZE    64
#define MAX_DATA_SIZE   128
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

static bool 
fdb_traverse_callback(void* state, const void* key, void* data)
{
    fdb_traversal_functor_state* fstate = NULL;
    fstate = state;
    assert(fstate);
    assert(key);

    return fstate->callback((fnode) data);
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
        fdb_log_db_state(db);
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

// TODO: ensure uniqueness of the db name
static void
fdb_set_name(fdb db, const char* name)
{
    assert(db);
    //assert(!db->name); /* name is immutable */

    // TODO: truncation possible
    snprintf(db->name, FDB_MAX_DB_NAME_LEN, "%s", name);
}

static inline void
fdb_print_content(const char* prefix, const void* content, size_t size)
{
    FDB_INFO("%s [%lu, %p]", prefix, size, content);
    hex_dump("content", (void *) content, size);
}

static void
fdb_node_header_print(struct node_header_* h)
{
   FDB_INFO("header:");
   FDB_INFO("\tnode_size:%lu", h->node_size);
   FDB_INFO("\tkey_size:%lu", h->key_size);
   FDB_INFO("\tdata_size:%lu", h->data_size);
   FDB_INFO("\tkey_type:%d", h->key_type);
   FDB_INFO("\tdata_type:%d", h->data_type);
}

static void
fdb_node_print(struct node_* n)
{
    if (n) {
        FDB_INFO("node_p=%p", n);
        fdb_node_header_print(&n->header);
        fdb_print_content("key", fnode_get_key(n), n->header.key_size);
        fdb_print_content("data", fnode_get_data(n), n->header.data_size);
    }
}

#if 0
static int
fdb_elem_cmp(struct felem* n1, struct felem* n2)
{
    int ret = 0;
    if (n1 == n2) {
        FDB_DEBUG("Same keys");
        return 0;
    }

    ret = n1->size - n2->size;
    if (ret != 0) {
        FDB_DEBUG("size mismatch => not equal: ret=%d", ret);
        return ret;
    }

    ret = memcmp(n1->content, n2->content, n1->size);
    return ret;
}
#endif

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

static int
key_cmp(const void* k1, const void* k2)
{
    int ret = 0;
    size_t k2_size;

    if (k1 == k2) {
        FDB_DEBUG("Same keys");
        return 0;
    }

    k2_size = fdb_node_get_header_from_key(k2)->key_size;

    //ret = k1_size - k2_size;
    //if (ret != 0) {
    //    FDB_DEBUG("size mismatch => not equal: ret=%d", ret);
    //    return ret;
   // }

    ret = memcmp(k1, k2, k2_size);
    return ret;
}

static void
fdb_node_free(fnode node)
{
    if (node) {
        fdb_node_print(node);
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
    size_t total_size = 0;

    assert(keysize > 0); /* data can be NULL */
    assert(keysize < MAX_KEY_SIZE);
    assert(datasize < MAX_DATA_SIZE);

    total_size = keysize + datasize + sizeof(struct node_header_);
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

void
fdb_node_log(fnode node)
{
    fdb_node_print(node);
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
    fnode        node = NULL;
    if (!db || !key || !data || (keysize == 0) || (datasize == 0)) {
        return false;
    }

    node = fdb_find(db, key, keysize);
    if (!node) {
        return false;
    }

    fdb_node_unset_data(node);
    node->header.data_size = datasize;
    fdb_node_set_data(node, data);

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

    // TODO: How to free data stored in the DB?
    // Should we register a destructor and call it?
    // Should we return to user
    // Should we allow the user to provide a "callback" in the remove call?

    // fdb_node_free(node);
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

size_t fdb_traverse(fdb db, traverse_cb callback)
{
    dict_visit_functor          functor;
    fdb_traversal_functor_state state;

    assert(db);
    assert(callback);

    state.callback = callback;
    functor.func = &fdb_traverse_callback;
    functor.data = &state;

    return dict_traverse_with_functor(db->dstore, &functor);
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
        return NULL;
    }

    if (!dict_itor_first(iter->it)) {
        FDB_ERROR("Cannot create iterator");
        dict_itor_free(iter->it);
        return NULL;
    }

    if (!dict_itor_valid(iter->it)) {
        FDB_ERROR("No elements in the iterator");
        dict_itor_free(iter->it);
        return NULL;
    }

    iter->next = (fnode) *dict_itor_datum(iter->it);
    return iter;
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
