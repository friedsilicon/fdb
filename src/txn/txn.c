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

static void
ftx_free(ftx txn)
{
    if (txn) {
        free(txn);
    }
}

static ftx
ftx_alloc()
{
    ftx txn = NULL;

    txn = calloc(1, sizeof(ftx_t));
    if (txn) {
        txn->db_count = 0;
    }

    return txn;
}

static bool
ftx_add_db(ftx tx, fdb db)
{
    assert(tx);
    assert(db);

    if (tx->db_count >= 10) return false;
    tx->dbs[tx->db_count++] = db;

    return true;
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
    bool ret = true;
    if (!txn) {
        return false;
    }

    for (int i = 0; i < txn->db_count; i++) {
        fdb db = txn->dbs[i];

        ret = db->prepare(txn, db);
        if (!ret) {
            // since atleast one callback failed, the txn is failed.
            FDB_DEBUG("Failed pre-commit for [%d].", db->id);
            break;
        }
    }

    if (!ret) {
        txn->state = TXN_ST_PREPARE_FAILED;
        return ret;
    }

    txn->state = TXN_ST_PREPARED;

    // pre-commit to DBs in this txn succeeded
    for (int i = 0; i < txn->db_count; i++) {
        fdb db = txn->dbs[i];

        ret = db->commit(txn, db);
        if (!ret) {
            txn->state = TXN_ST_COMMIT_FAILED;
            FDB_ERROR("Failed commit for [%d].", db->id);
            // failed but keep trying other participants in txn
        }
    }

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
    bool ret = true;
    if (!txn) {
        return false;
    }

    for (int i = 0; i < txn->db_count; i++) {
        fdb db = txn->dbs[i];

        ret = db->abort(txn, db);
        if (!ret) {
            FDB_DEBUG("Failed abort for [%d].", db->id);
            txn->state = TXN_ST_ABORT_FAILED;
        }
    }

    return (txn->state == TXN_ST_ABORT_FAILED) ? false : true;
}

void
fdb_txn_finish(ftx txn)
{
    if (!txn) {
        return;
    }

    if ((txn->state == TXN_ST_PREPARE_FAILED) ||
        (txn->state == TXN_ST_COMMIT_FAILED)) {
        fdb_txn_abort(txn);
    }

    for (int i = 0; i < txn->db_count; i++) {
        fdb db = txn->dbs[i];
        db->finish(txn, db);
    }

    ftx_free(txn);
    txn = NULL;
    return;
}
