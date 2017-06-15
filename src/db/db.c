#define _POSIX_C_SOURCE 200809L

#include "fdb_private.h"
#include "fdb_log.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 32

static int fdb_id = 1;

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
        free(db->name);
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
    assert(!db->name); /* name is immutable */

    db->name = strndup(name, MAX_NAME_LEN);
}

static inline void
fdb_item_print(struct felem* e, const char* prefix)
{
    FDB_DEBUG("%s [%lu, %s]", prefix, e->size, e->content);
}

static void
fdb_node_print(struct node_* n)
{
    if (n) {
        FDB_DEBUG("node_p=%p", n);
        fdb_item_print(&n->key, "key");
        fdb_item_print(&n->data, "data");
    }
}

void
fdb_node_log(fnode node)
{
    fdb_node_print(node);
}

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

static int
key_cmp(const void* k1, const void* k2)
{
    return fdb_elem_cmp((struct felem*) k1, (struct felem*) k2);
}

static void
fdb_item_free(struct felem* item)
{
    if (item) {
        if (item->content) {
            free(item->content);
            item->content = NULL;
            item->size = 0;
        }
        /* don't free item itself (it is part of node) */
    }
}

static void
fdb_item_copy(struct felem* src, struct felem* dest)
{
    assert(src);
    assert(dest);

    dest->content = src->content;
    dest->size = src->size;
}

static void
fdb_node_free(fnode node)
{
    if (node) {
        fdb_item_free(&node->key);
        fdb_item_free(&node->data);
        free(node);
    }
}

static void
key_val_free(void* key, void* datum)
{
    /* just free the data, since it contains the key */
    fdb_node_free((fnode) datum);
}

static bool
fdb_make_item(struct felem* item, size_t size)
{
    assert(item);

    item->content = calloc(1, size);
    if (!item->content) {
        return false;
    }

    item->size = size;
    return true;
}

static struct node_*
fdb_make_node(size_t keysize, size_t datasize)
{
    struct node_* node = NULL;

    assert(keysize > 0); /* data can be NULL */

    node = calloc(1, sizeof(struct node_));
    if (node) {

        if (!fdb_make_item(&node->key, keysize)) {
            fdb_node_free(node);
            node = NULL;
        }

        if (!fdb_make_item(&node->data, datasize)) {
            fdb_node_free(node);
            node = NULL;
        }
    }

    return node;
}

static void
fdb_item_set_content(struct felem* elem, const void* content)
{
    assert(elem);
    assert(content);

    memmove(elem->content, content, elem->size);
}

static void
fdb_node_set_key(struct node_* node, const void* key)
{
    assert(node);
    fdb_item_set_content(&node->key, key);
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
    db->dstore = rb_dict_new(key_cmp, key_val_free);
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
fdb_deinit(fdb* db)
{
    fdb t = *db;
    if (t && t->dstore) {
        dict_free(t->dstore);
        t->dstore = NULL;
    }
    fdb_free(t);
    *db = NULL;
}

bool
fdb_insert(fdb db, key key, size_t keysize, data data, size_t datasize)
{
    struct node_* node = NULL;
    void**        datum_loc;
    bool          ret;
    if (!db || !key || !data || (keysize == 0) || (datasize == 0)) {
        return false;
    }

    node = fdb_make_node(keysize, datasize);
    if (!node) {
        return false;
    }

    fdb_node_set_key(node, key);
    fdb_node_set_data(node, data);
    datum_loc = dict_insert(db->dstore, &node->key, &ret);
    if (ret) {
        *datum_loc = node;
    }

    return ret;
}

bool
fdb_update(fdb db, key key, size_t keysize, data data, size_t datasize)
{
    fnode        node = NULL;
    struct felem item;
    if (!db || !key || !data || (keysize == 0) || (datasize == 0)) {
        return false;
    }

    node = fdb_find(db, key, keysize);
    if (!node) {
        return false;
    }

    if (!fdb_make_item(&item, datasize)) {
        return false;
    }

    fdb_item_set_content(&item, data);
    fdb_item_free(&node->data);
    fdb_item_copy(&item, &node->data);

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
    struct felem item;
    bool         ret = false;

    if (!db || !key || (keysize == 0)) {
        FDB_ERROR("Invalid Arguments");
        return false;
    }

    node = fdb_find(db, key, keysize);
    if (!node) {
        // no such node
        return false;
    }

    ret = dict_remove(db->dstore, &node->key);
    if (!ret) {
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
    struct felem item;

    if (!db || !key || (keysize == 0)) {
        FDB_ERROR("Invalid Arguments");
        return NULL;
    }

    /* We are not going to modify the key.
     * Discard after search
     */
    item.size = keysize;
    item.content = (void*) key;

    return dict_search(db->dstore, &item);
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

    iter->next = (fnode) *dict_itor_data(iter->it);
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
        iter->next = (fnode) *dict_itor_data(iter->it);
    }

    return current;
}

void*
felem_get_content(struct felem* elem)
{
    return elem->content;
}

size_t
felem_get_size(struct felem* elem)
{
    return elem->size;
}

key
fnode_get_key(fnode node)
{
    if (!node) {
        return NULL;
    }

    return felem_get_content(&node->key);
}

size_t
fnode_get_keysize(fnode node)
{
    if (!node) {
        return 0;
    }

    return felem_get_size(&node->key);
}

data
fnode_get_data(fnode node)
{
    if (!node) {
        return NULL;
    }

    return felem_get_content(&node->data);
}

size_t
fnode_get_datasize(fnode node)
{
    if (!node) {
        return 0;
    }

    return felem_get_size(&node->data);
}

void
fdb_node_set_data(struct node_* node, data data)
{
    assert(node);
    fdb_item_set_content(&node->data, data);
}

