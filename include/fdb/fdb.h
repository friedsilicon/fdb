/**
 * @file fdb.h
 * @brief Main FDB public interface.
 *
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#ifndef FDB_H_
#define FDB_H_

#include "fdb/fdb_types.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration — full definition in fdb_storage.h (internal).    */
typedef struct fdb_storage_ops_ fdb_storage_ops_t;

/* ---------------------------------------------------------------------- */
/* Initialisation                                                         */
/* ---------------------------------------------------------------------- */

/* Create a database using the built-in in-memory backend.               */
fdb fdb_init(const char* name);

/* Create a database with a caller-supplied storage backend.             */
/* ops_ctx must point to a buffer of at least ops->ctx_size bytes;       */
/* fdb does not take ownership of ops_ctx.                               */
fdb fdb_init_with_storage(const char* name,
                          const fdb_storage_ops_t* ops,
                          void* ops_ctx);

void fdb_deinit(fdb db);

/* ---------------------------------------------------------------------- */
/* Key-value operations                                                   */
/* ---------------------------------------------------------------------- */

bool fdb_insert(fdb db, const void* key, size_t keysize,
                         const void* data, size_t datasize);

bool fdb_update(fdb db, const void* key, size_t keysize,
                         const void* data, size_t datasize);

bool fdb_exists(fdb db, const void* key, size_t keysize);
bool fdb_remove(fdb db, const void* key, size_t keysize);

/* Returns FDB_NODE_INVALID (key == NULL) when not found.                */
fdb_node_t fdb_find(fdb db, const void* key, size_t keysize);

/* ---------------------------------------------------------------------- */
/* Iteration                                                              */
/* ---------------------------------------------------------------------- */

fiter      fdb_iterate(fdb db);
bool       fiter_hasnext(fiter iter);
fdb_node_t fiter_next(fiter iter);
void       fiter_free(fiter iter);

/* ---------------------------------------------------------------------- */
/* Traversal                                                              */
/* ---------------------------------------------------------------------- */

typedef bool (*fdb_traverse_cb)(fdb_node_t node, void* user_data);
size_t fdb_traverse(fdb db, fdb_traverse_cb callback, void* user_data);

/* ---------------------------------------------------------------------- */
/* Transaction API                                                        */
/* ---------------------------------------------------------------------- */

ftx  fdb_txn_start(fdb db);
bool fdb_txn_join(ftx txn, fdb db);
bool fdb_txn_commit(ftx txn);
bool fdb_txn_abort(ftx txn);
void fdb_txn_finish(ftx txn);

/* ---------------------------------------------------------------------- */
/* Persistence                                                            */
/* ---------------------------------------------------------------------- */

bool fdb_save(fdb db, const char* filename);
bool fdb_load(fdb db, const char* filename);

#ifdef __cplusplus
}
#endif

#endif /* FDB_H_ */
