/**
 * @file fdb.h
 * @brief Main FDB Interface
 * @author Shivanand Velmurugan
 * @version 1.0
 * @date 2016-11-01
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

fdb fdb_init(const char* name);
fdb fdb_init_advanced(const char* name, fdb_type_t type);
void fdb_deinit(fdb db);

bool fdb_insert(fdb db, key key, size_t keysize, data data, size_t datasize);
bool fdb_update(fdb db, key key, size_t keysize, data data, size_t datasize);

bool fdb_exists(fdb db, key key, size_t keysize);
bool fdb_remove(fdb db, key key, size_t keysize);

fnode fdb_find(fdb db, key key, size_t keysize);

fiter fdb_iterate(fdb db);
bool fiter_hasnext(fiter iter);
fnode fiter_next(fiter iter);

typedef bool (*traverse_cb)(fnode node, void* user_data);
size_t fdb_traverse(fdb db, traverse_cb callback, void* user_data);

key fnode_get_key(fnode node);
size_t fnode_get_keysize(fnode node);
data fnode_get_data(fnode node);
size_t fnode_get_datasize(fnode node);

/* txn api */
ftx fdb_txn_start(fdb db);
bool fdb_txn_join(ftx txn, fdb db);
bool fdb_txn_commit(ftx txn);
bool fdb_txn_abort(ftx txn);
void fdb_txn_finish(ftx txn);

/* persistence */
bool fdb_save(fdb db, const char* filename);
bool fdb_load(fdb db, const char* filename);

/* logging */
void fdb_node_log(fnode node);

#ifdef __cplusplus
}
#endif

#endif /* FDB_H_ */
