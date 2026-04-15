/*
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 *
 * Core fdb implementation.  All storage is delegated to the pluggable
 * fdb_storage_ops_t backend; this file contains no direct libdict calls.
 */

#define _POSIX_C_SOURCE 200809L

#include "fdb_private.h"
#include "fdb_log.h"
#include "mem_backend.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>

/* ---------------------------------------------------------------------- */
/* Global db-id counter (not thread-safe yet)                             */
/* ---------------------------------------------------------------------- */

#define FDB_DB_ID_MIN     1
#define FDB_DB_ID_INVALID (FDB_DB_ID_MIN - 1)

static int fdb_id = FDB_DB_ID_MIN;

/* ---------------------------------------------------------------------- */
/* 2PC stub callbacks                                                     */
/* ---------------------------------------------------------------------- */

static bool join_tx   (ftx tx, fdb db) { (void)tx; (void)db; return true; }
static bool prepare_tx(ftx tx, fdb db) { (void)tx; (void)db; return true; }
static bool commit_tx (ftx tx, fdb db) { (void)tx; (void)db; return true; }
static bool abort_tx  (ftx tx, fdb db) { (void)tx; (void)db; return true; }
static bool finish_tx (ftx tx, fdb db) { (void)tx; (void)db; return true; }

/* ---------------------------------------------------------------------- */
/* Internal helpers                                                       */
/* ---------------------------------------------------------------------- */

bool
is_fdb_handle_valid(fdb db)
{
    return db &&
           db->join && db->prepare && db->commit && db->abort && db->finish;
}

void
fdb_log_db_state(const fdb db)
{
    FDB_DEBUG("id=%d, type=%d, name=%s", db->id, db->type, db->name);
}

static fdb
fdb_alloc_handle(const char* name, fdb_type_t type)
{
    fdb db = calloc(1, sizeof(fdb_t));
    if (!db) return NULL;

    db->id   = fdb_id++;
    db->type = type;
    snprintf(db->name, sizeof(db->name), "%s", name);

    db->join    = join_tx;
    db->prepare = prepare_tx;
    db->commit  = commit_tx;
    db->abort   = abort_tx;
    db->finish  = finish_tx;

    return db;
}

static void
fdb_free_handle(fdb db)
{
    free(db);
}

/* ---------------------------------------------------------------------- */
/* Public — initialisation                                                */
/* ---------------------------------------------------------------------- */

fdb
fdb_init(const char* name)
{
    if (!name) return NULL;

    const fdb_storage_ops_t* ops = fdb_mem_backend();

    void* ctx = calloc(1, ops->ctx_size);
    if (!ctx) return NULL;

    if (!ops->open(ctx)) {
        free(ctx);
        return NULL;
    }

    fdb db = fdb_alloc_handle(name, FDB_SINGLE_INDEX_DB);
    if (!db) {
        ops->close(ctx);
        free(ctx);
        return NULL;
    }

    db->ops              = ops;
    db->storage_ctx      = ctx;
    db->owns_storage_ctx = true;
    return db;
}

fdb
fdb_init_with_storage(const char* name,
                      const fdb_storage_ops_t* ops,
                      void* ops_ctx)
{
    if (!name || !ops || !ops_ctx) return NULL;

    fdb db = fdb_alloc_handle(name, FDB_SINGLE_INDEX_DB);
    if (!db) return NULL;

    db->ops              = ops;
    db->storage_ctx      = ops_ctx;
    db->owns_storage_ctx = false;

    if (!ops->open(ops_ctx)) {
        fdb_free_handle(db);
        return NULL;
    }

    return db;
}

void
fdb_deinit(fdb db)
{
    if (!db) return;

    if (db->ops && db->storage_ctx) {
        db->ops->close(db->storage_ctx);
        if (db->owns_storage_ctx) {
            free(db->storage_ctx);
        }
        db->storage_ctx = NULL;
    }

    fdb_free_handle(db);
}

/* ---------------------------------------------------------------------- */
/* Public — key-value mutations                                           */
/* ---------------------------------------------------------------------- */

bool
fdb_insert(fdb db, const void* key, size_t keysize,
                   const void* data, size_t datasize)
{
    if (!db || !key || !data || keysize == 0 || datasize == 0) return false;
    return db->ops->insert(db->storage_ctx, key, keysize, data, datasize);
}

bool
fdb_update(fdb db, const void* key, size_t keysize,
                   const void* data, size_t datasize)
{
    if (!db || !key || !data || keysize == 0 || datasize == 0) return false;
    return db->ops->update(db->storage_ctx, key, keysize, data, datasize);
}

bool
fdb_remove(fdb db, const void* key, size_t keysize)
{
    if (!db || !key || keysize == 0) return false;
    return db->ops->remove(db->storage_ctx, key, keysize);
}

/* ---------------------------------------------------------------------- */
/* Public — reads                                                         */
/* ---------------------------------------------------------------------- */

fdb_node_t
fdb_find(fdb db, const void* key, size_t keysize)
{
    if (!db || !key || keysize == 0) return FDB_NODE_INVALID;
    return db->ops->find(db->storage_ctx, key, keysize);
}

bool
fdb_exists(fdb db, const void* key, size_t keysize)
{
    return fdb_node_valid(fdb_find(db, key, keysize));
}

/* ---------------------------------------------------------------------- */
/* Public — traversal                                                     */
/* ---------------------------------------------------------------------- */

size_t
fdb_traverse(fdb db, fdb_traverse_cb callback, void* user_data)
{
    if (!db || !callback) return 0;
    /* fdb_traverse_cb and fdb_storage_visit_fn share the same signature. */
    return db->ops->traverse(db->storage_ctx,
                             (fdb_storage_visit_fn)callback,
                             user_data);
}

/* ---------------------------------------------------------------------- */
/* Public — iteration                                                     */
/* ---------------------------------------------------------------------- */

fiter
fdb_iterate(fdb db)
{
    if (!db) return NULL;

    /* Allocate iterator header + cursor buffer in one shot. */
    void* cursor_buf = calloc(1, db->ops->cursor_buf_size);
    if (!cursor_buf) return NULL;

    if (!db->ops->cursor_open(db->storage_ctx, cursor_buf)) {
        free(cursor_buf);
        return NULL;
    }

    if (!db->ops->cursor_first(cursor_buf) ||
        !db->ops->cursor_valid(cursor_buf))
    {
        /* Empty db — close cursor and report no iterator. */
        db->ops->cursor_close(cursor_buf);
        free(cursor_buf);
        return NULL;
    }

    fiter iter = calloc(1, sizeof(struct fdb_iter_));
    if (!iter) {
        db->ops->cursor_close(cursor_buf);
        free(cursor_buf);
        return NULL;
    }

    iter->ops        = db->ops;
    iter->cursor_buf = cursor_buf;
    return iter;
}

bool
fiter_hasnext(fiter iter)
{
    if (!iter) return false;
    return iter->ops->cursor_valid(iter->cursor_buf);
}

fdb_node_t
fiter_next(fiter iter)
{
    if (!iter || !iter->ops->cursor_valid(iter->cursor_buf))
        return FDB_NODE_INVALID;

    fdb_node_t node = iter->ops->cursor_get(iter->cursor_buf);
    iter->ops->cursor_next(iter->cursor_buf);
    return node;
}

void
fiter_free(fiter iter)
{
    if (!iter) return;
    if (iter->cursor_buf) {
        iter->ops->cursor_close(iter->cursor_buf);
        free(iter->cursor_buf);
    }
    free(iter);
}
