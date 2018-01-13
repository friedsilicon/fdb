/*
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#include "fdb_private.h"
#include "CppUTest/TestHarness.h"
#include "fdb/fdb.h"
#include <stdio.h>
#include <string.h>

// clang-format off
TEST_GROUP(FdbSerializeGroup)
{
    fdb db;

    void setup()
    {
        db = fdb_init("foo");
    }

    void teardown()
    {
        fdb_deinit(db);
    }

    void check_node(const fnode n,
                    const char* k, size_t kl,
                    const char* v, size_t vl)
    {
        CHECK(n != NULL);
        //fdb_node_log(n);

        STRCMP_EQUAL(k, (char*) fnode_get_key(n));
        CHECK_EQUAL(kl, fnode_get_keysize(n));
        STRCMP_EQUAL(v, (char*) fnode_get_data(n));
        CHECK_EQUAL(vl, fnode_get_datasize(n));
    }

    void check_record(fdb db_inst, const char* k, const char* v)
    {
        size_t kl, vl;

        kl = strlen(k);
        vl = strlen(v);

        fnode n = fdb_find(db_inst, k, kl+1);
        check_node(n, k, kl+1, v, vl+1);
    }
};
// clang-format on

TEST(FdbSerializeGroup, Save)
{
    CHECK(fdb_insert(db, "key1", 5, "data1", 6));
    check_record(db, "key1", "data1");
    CHECK(fdb_insert(db, "key2", 5, "data2", 6));
    check_record(db, "key2", "data2");
    CHECK(fdb_insert(db, "key3", 5, "data3", 6));
    check_record(db, "key3", "data3");

    CHECK(fdb_save(db, "./fdb-serialize.bin"));
    remove("./fdb-serialize.bin");
}

TEST(FdbSerializeGroup, SaveLoad)
{
    CHECK(fdb_insert(db, "key1", 5, "data1", 6));
    CHECK(fdb_insert(db, "key2", 5, "data2", 6));
    CHECK(fdb_insert(db, "key3", 5, "data3", 6));

    check_record(db, "key1", "data1");
    check_record(db, "key2", "data2");
    check_record(db, "key3", "data3");

    CHECK(fdb_save(db, "./fdb-serialize.bin"));
    
    fdb_deinit(db);
    db = NULL;

    db = fdb_init("foo");
    CHECK(fdb_load(db, "./fdb-serialize.bin"));

    check_record(db, "key1", "data1");
    check_record(db, "key2", "data2");
    check_record(db, "key3", "data3");
    fdb_deinit(db);
}

