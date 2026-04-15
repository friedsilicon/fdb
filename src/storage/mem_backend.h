/**
 * @file mem_backend.h
 * @brief In-memory storage backend for fdb (backed by a red-black tree).
 *
 * This is the default backend used by fdb_init().  It keeps all data in
 * heap memory; nothing is persisted unless fdb_save() is called explicitly.
 *
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#ifndef FDB_MEM_BACKEND_H_
#define FDB_MEM_BACKEND_H_

#include "fdb_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the ops table for the in-memory backend.
 * The returned pointer is to a statically allocated struct — no free needed. */
const fdb_storage_ops_t* fdb_mem_backend(void);

#ifdef __cplusplus
}
#endif

#endif /* FDB_MEM_BACKEND_H_ */
