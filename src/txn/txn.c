#define _POSIX_C_SOURCE 200809L

#include "fdb/fdb.h"
#include "fdb_private.h"
#include "fdb_log.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int txn_id = 1;

static int
key_cmp(const void* k1, const void* k2)
{
    int32_t one = *(int32_t*) k1;
    int32_t two = *(int32_t*) k2;

    int diff = one - two;

    if (diff < 0)
        return -1;
    if (diff > 0)
        return 1;
    return 0;
}

static void
txn_db_list_delete(void* dbid, void* db)
{
    if (db) {
        fdb_log_db_state((fdb) db);
        fdb_deinit( (fdb) db);
    }
}

static void
ftx_free(ftx txn)
{
    if (txn) {
        if (txn->fdb_list) {
            dict_free(txn->fdb_list, txn_db_list_delete);
        }
        free(txn);
    }
}

static ftx
ftx_alloc()
{
    ftx txn = NULL;

    txn = calloc(1, sizeof(ftx_t));
    if (txn) {
        txn->fdb_list = skiplist_dict_new(key_cmp, 10);

        if (!txn->fdb_list) {
            ftx_free(txn);
            txn = NULL;
        }
    }

    return txn;
}

static bool
ftx_add_db(ftx tx, fdb db)
{
    dict_insert_result result;

    assert(tx);
    assert(tx->fdb_list);
    assert(db);

    // TODO: should we return fail if found?
    result = dict_insert(tx->fdb_list, &db->id);
    if (result.inserted) {
        *result.datum_ptr = db;
    }

    return result.inserted;
}

ftx
fdb_txn_start(fdb db)
{
    ftx tx = NULL;

    if (!db) {
        return NULL;
    }

    tx = ftx_alloc();
    if (!tx) {
        return NULL;
    }

    tx->id = txn_id;
    if (!fdb_txn_join(tx, db)) {
        FDB_ERROR("Cannot start txn [%d] for db [%d]", tx->id, db->id);
        fdb_txn_finish(tx);
        return NULL;
    }

    // TODO: thread safety
    txn_id++; /* increment global counter */
    return tx;
}

bool
fdb_txn_join(ftx txn, fdb db)
{
    bool ret = true;
    if (!txn || !db || !is_fdb_handle_valid(db)) {
        return false;
    }

    ret = db->join(txn, db);
    if (ret) {
        ret = ftx_add_db(txn, db);
        assert(ret);
    }
    return ret;
}

bool
fdb_txn_commit(ftx txn)
{
    bool       ret = true;
    dict_itor* itor;
    if (!txn) {
        return false;
    }

    itor = dict_itor_new(txn->fdb_list);
    dict_itor_first(itor);
    for (; dict_itor_valid(itor); dict_itor_next(itor)) {
        fdb db = (fdb) *dict_itor_datum(itor);

        FDB_DEBUG("'%d': '%p'\n", *(int*) dict_itor_key(itor), db);

        ret = db->prepare(txn, db);
        if (!ret) {
            // since atleast one callback failed, the txn is failed.
            FDB_DEBUG("Failed pre-commit for [%d].", db->id);
            break;
        }
    }
    dict_itor_free(itor);

    if (!ret) {
        txn->state = TXN_ST_PREPARE_FAILED;
        return ret;
    }

    txn->state = TXN_ST_PREPARED;

    // pre-commit to DBs in this txn succeeded
    itor = dict_itor_new(txn->fdb_list);
    dict_itor_first(itor);
    for (; dict_itor_valid(itor); dict_itor_next(itor)) {
        fdb db = (fdb) *dict_itor_datum(itor);

        FDB_DEBUG("'%d': '%p'\n", *(int*) dict_itor_key(itor), db);

        ret = db->commit(txn, db);
        if (!ret) {
            txn->state = TXN_ST_COMMIT_FAILED;
            FDB_ERROR("Failed commit for [%d].", db->id);
            // failed but keep trying other participants in txn
        }
    }
    dict_itor_free(itor);

    if (txn->state == TXN_ST_COMMIT_FAILED) {
        return false;
    } else {
        txn->state = TXN_ST_COMMITTED;
        return true;
    }
}

bool
fdb_txn_abort(ftx txn)
{
    bool       ret = true;
    dict_itor* itor;
    if (!txn) {
        return false;
    }

    itor = dict_itor_new(txn->fdb_list);
    dict_itor_first(itor);
    for (; dict_itor_valid(itor); dict_itor_next(itor)) {
        fdb db = *(fdb *) dict_itor_datum(itor);

        FDB_DEBUG("'%d': '%p'\n", *(int32_t*) dict_itor_key(itor), db);
        fdb_log_db_state(db);
        ret = db->abort(txn, db);
        if (!ret) {
            FDB_DEBUG("Failed abort for [%d].", db->id);
            txn->state = TXN_ST_ABORT_FAILED;
        }
    }
    dict_itor_free(itor);

    return (txn->state == TXN_ST_ABORT_FAILED) ? false : true;
}

void
fdb_txn_finish(ftx txn)
{
    dict_itor* itor;
    if (!txn) {
        return;
    }

    if ((txn->state == TXN_ST_PREPARE_FAILED) ||
        (txn->state == TXN_ST_COMMIT_FAILED)) {
        fdb_txn_abort(txn);
    }

    itor = dict_itor_new(txn->fdb_list);
    dict_itor_first(itor);
    for (; dict_itor_valid(itor); dict_itor_next(itor)) {
        fdb db = *(fdb *) dict_itor_datum(itor);
        FDB_DEBUG("'%d': '%p'\n", *(int32_t*) dict_itor_key(itor), db);
        db->finish(txn, db);
    }
    dict_itor_free(itor);

    itor = NULL;
    ftx_free(txn);
    txn = NULL;
    return;
}
