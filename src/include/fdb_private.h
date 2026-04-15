/**
 * @file fdb_private.h
 * @brief Internal types shared across fdb implementation files.
 *
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#ifndef FDB_PRIVATE_H_
#define FDB_PRIVATE_H_

#include "fdb/fdb.h"
#include "fdb_storage.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#define FDB_MAX_DB_NAME_LEN  31
#define FDB_MAX_DB_NAME_SIZE (FDB_MAX_DB_NAME_LEN + 1)

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */
/* Transaction callback type (used by 2PC coordinator)                    */
/* ---------------------------------------------------------------------- */

typedef bool (*callback_fn)(ftx tx, fdb db);

/* ---------------------------------------------------------------------- */
/* Database handle                                                        */
/* ---------------------------------------------------------------------- */

struct fdb_
{
    /* identity */
    int32_t    id;
    fdb_type_t type;
    char       name[FDB_MAX_DB_NAME_SIZE];

    /* pluggable storage backend */
    const fdb_storage_ops_t* ops;
    void*                    storage_ctx;      /* ops->ctx_size bytes        */
    bool                     owns_storage_ctx; /* true when fdb allocated it */

    /* persistence (optional) */
    fdb_file   file_on_disk;

    /* 2PC transaction callbacks */
    callback_fn join;
    callback_fn prepare;
    callback_fn commit;
    callback_fn abort;
    callback_fn finish;
};

/* ---------------------------------------------------------------------- */
/* Iterator handle                                                        */
/* ---------------------------------------------------------------------- */

struct fdb_iter_
{
    const fdb_storage_ops_t* ops;
    void*                    cursor_buf; /* ops->cursor_buf_size bytes, owned */
};

/* ---------------------------------------------------------------------- */
/* Transaction handle                                                     */
/* ---------------------------------------------------------------------- */

typedef enum tstate_e {
    TXN_ST_NEW,            /* new and clean                  */
    TXN_ST_LIVE,           /* live — not yet validated       */
    TXN_ST_PREPARED,       /* ready for commit               */
    TXN_ST_ABORTED,        /* aborted                        */
    TXN_ST_COMMITTED,      /* committed                      */
    TXN_ST_PREPARE_FAILED, /* validation failed              */
    TXN_ST_COMMIT_FAILED,  /* commit failed                  */
    TXN_ST_ABORT_FAILED    /* abort failed                   */
} tstate_t;

struct ftx_
{
    int      id;
    tstate_t state;
    int      db_count;
    fdb      dbs[10];
};

/* ---------------------------------------------------------------------- */
/* Helpers                                                                */
/* ---------------------------------------------------------------------- */

bool is_fdb_handle_valid(fdb db);
void fdb_log_db_state(const fdb db);

#ifdef __cplusplus
}
#endif

#endif /* FDB_PRIVATE_H_ */
