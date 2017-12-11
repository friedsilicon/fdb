/*
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "fdb/fdb.h"
#include "fdb_private.h"
#include <stdio.h>
#include <string.h>

// clang-format off
TEST_GROUP(FdbTxnGroup)
{
    void setup(){}

    void teardown(){}

    void setupCallback(fdb db, callback_fn fn)
    {
        db->join    = fn;
        db->prepare = fn;
        db->commit  = fn;
        db->abort   = fn;
        db->finish  = fn;
    }

    void setupDB(fdb db, int id, callback_fn fn)
    {
        memset(db, 0, sizeof(fdb_t));
        db->id = id;
        db->type = FDB_SINGLE_INDEX_DB;
        db->idx_type = DB_SINGLE_INDEX;
        snprintf(db->name, FDB_MAX_DB_NAME_SIZE, "%s%d", "db", id); 
        setupCallback(db, fn);
    }
};
// clang-format on

bool
test_callback(ftx tx, fdb db)
{
    return mock().actualCall("test_callback").returnBoolValue();
}

bool
test_2_callback(ftx tx, fdb db)
{
    return mock().actualCall("test_2_callback").returnBoolValue();
}

bool
test_3_callback(ftx tx, fdb db)
{
    return mock().actualCall("test_3_callback").returnBoolValue();
}

TEST(FdbTxnGroup, TxnOpsInvalid)
{
    CHECK(NULL == fdb_txn_start(NULL));
    CHECK_FALSE(fdb_txn_join(NULL, NULL));
    CHECK_FALSE(fdb_txn_commit(NULL));
    CHECK_FALSE(fdb_txn_abort(NULL));
    fdb_txn_finish(NULL);
}

TEST(FdbTxnGroup, TxnStartAndJoinInvalid)
{
    ftx tx;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));

    db1->id = 10;
    db1->type = FDB_SINGLE_INDEX_DB;
    db1->idx_type = DB_SINGLE_INDEX;
    setupCallback(db1, &test_callback);

    db2->id = 11;
    db2->type = FDB_SINGLE_INDEX_DB;
    db2->idx_type = DB_SINGLE_INDEX;
    setupCallback(db2, &test_2_callback);

    CHECK_FALSE(fdb_txn_join(NULL, db2));
    CHECK_FALSE(fdb_txn_join(tx, NULL));

    mock().expectOneCall("test_callback").andReturnValue(false);
    CHECK(NULL == fdb_txn_start(db1));

    mock().expectOneCall("test_callback").andReturnValue(true);
    tx = fdb_txn_start(db1);
    CHECK(tx != NULL);

    /* valid txn started */

    mock().expectOneCall("test_2_callback").andReturnValue(false);
    CHECK_FALSE(fdb_txn_join(tx, db2));

    mock().expectOneCall("test_2_callback").andReturnValue(true);
    CHECK(fdb_txn_join(tx, db2));

    // CHECK_FALSE(fdb_txn_join(tx, &db2)); // try adding duplicate database to
    // txn

    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    fdb_txn_finish(tx);
}

TEST(FdbTxnGroup, TxnStartAndJoin)
{
    ftx   tx;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    memset(db1, 0, sizeof(fdb_t));
    memset(db2, 0, sizeof(fdb_t));
    memset(db3, 0, sizeof(fdb_t));

    db1->id = 10;
    db1->type = FDB_SINGLE_INDEX_DB;
    db1->idx_type = DB_SINGLE_INDEX;
    setupCallback(db1, &test_callback);

    db2->id = 11;
    db2->type = FDB_SINGLE_INDEX_DB;
    db2->idx_type = DB_SINGLE_INDEX;
    setupCallback(db2, &test_2_callback);

    db3->id = 12;
    db3->type = FDB_SINGLE_INDEX_DB;
    db3->idx_type = DB_SINGLE_INDEX;
    setupCallback(db3, &test_3_callback);

    // create txn with a valid db
    mock().expectOneCall("test_callback").andReturnValue(true);
    tx = fdb_txn_start(db1);
    CHECK(tx != NULL);

    mock().expectOneCall("test_3_callback").andReturnValue(true);
    CHECK(fdb_txn_join(tx, db3));

    mock().expectOneCall("test_2_callback").andReturnValue(true);
    CHECK(fdb_txn_join(tx, db2));

    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(true);
    fdb_txn_finish(tx);
}

TEST(FdbTxnGroup, TxnCommit)
{
    ftx   tx;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    // create txn with a valid db
    mock().expectOneCall("test_callback").andReturnValue(true);
    tx = fdb_txn_start(db1);
    CHECK(tx != NULL);

    mock().expectOneCall("test_2_callback").andReturnValue(true);
    CHECK(fdb_txn_join(tx, db2));

    mock().expectOneCall("test_3_callback").andReturnValue(true);
    CHECK(fdb_txn_join(tx, db3));

    // called for prepare
    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(true);

    // called for commit
    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(true);
    CHECK(fdb_txn_commit(tx));

    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(true);
    fdb_txn_finish(tx);
}

TEST(FdbTxnGroup, TxnCommitFailedDuringPrepare)
{
    ftx   tx;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    // create txn with a valid db
    mock().expectOneCall("test_callback").andReturnValue(true);
    tx = fdb_txn_start(db1);
    CHECK(tx != NULL);

    mock().expectOneCall("test_2_callback").andReturnValue(true);
    CHECK(fdb_txn_join(tx, db2));

    mock().expectOneCall("test_3_callback").andReturnValue(true);
    CHECK(fdb_txn_join(tx, db3));

    // fail prepare for DB3
    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(false);
    CHECK_FALSE(fdb_txn_commit(tx));

    // calls for abort
    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(true);

    // calls for finish
    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(true);
    fdb_txn_finish(tx);
}

TEST(FdbTxnGroup, TxnCommitFailedDuringCommit)
{
    ftx   tx;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    // create txn with a valid db
    mock().expectOneCall("test_callback").andReturnValue(true);
    tx = fdb_txn_start(db1);
    CHECK(tx != NULL);

    mock().expectOneCall("test_2_callback").andReturnValue(true);
    CHECK(fdb_txn_join(tx, db2));

    mock().expectOneCall("test_3_callback").andReturnValue(true);
    CHECK(fdb_txn_join(tx, db3));

    // prepare succeeds
    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(true);

    // but commit fails for db 2
    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(false);
    CHECK_FALSE(fdb_txn_commit(tx));

    // calls for abort
    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(true);

    // calls for finish
    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(true);
    fdb_txn_finish(tx);
}

TEST(FdbTxnGroup, TxnAbort)
{
    ftx   tx;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    // create txn with a valid db
    mock().expectOneCall("test_callback").andReturnValue(true);
    tx = fdb_txn_start(db1);
    CHECK(tx != NULL);

    mock().expectOneCall("test_2_callback").andReturnValue(true);
    CHECK(fdb_txn_join(tx, db2));

    mock().expectOneCall("test_3_callback").andReturnValue(true);
    CHECK(fdb_txn_join(tx, db3));

    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(true);
    CHECK(fdb_txn_abort(tx));

    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(true);
    fdb_txn_finish(tx);
}

TEST(FdbTxnGroup, TxnAbortFails)
{
    ftx   tx;
    fdb db1 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db2 = (fdb) calloc(1, sizeof(fdb_t));
    fdb db3 = (fdb) calloc(1, sizeof(fdb_t));

    setupDB(db1, 10, &test_callback);
    setupDB(db2, 11, &test_2_callback);
    setupDB(db3, 12, &test_3_callback);

    // create txn with a valid db
    mock().expectOneCall("test_callback").andReturnValue(true);
    tx = fdb_txn_start(db1);
    CHECK(tx != NULL);

    mock().expectOneCall("test_2_callback").andReturnValue(true);
    CHECK(fdb_txn_join(tx, db2));

    mock().expectOneCall("test_3_callback").andReturnValue(true);
    CHECK(fdb_txn_join(tx, db3));

    //fdb_insert(&db1, "1", 2, "data1", 6);
    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(false);
    CHECK_FALSE(fdb_txn_abort(tx));

    mock().expectOneCall("test_callback").andReturnValue(true);
    mock().expectOneCall("test_2_callback").andReturnValue(true);
    mock().expectOneCall("test_3_callback").andReturnValue(true);
    fdb_txn_finish(tx);
}
