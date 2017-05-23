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
        fdb_deinit(&db);
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

    void check_record(const char* k, const char* v)
    {
        size_t kl, vl;

        kl = strlen(k);
        vl = strlen(v);

        fnode n = fdb_find(db, k, kl);
        check_node(n, k, kl, v, vl);
    }
};
// clang-format on

TEST(FdbSerializeGroup, Save)
{
    CHECK(fdb_insert(db, "key1", 4, "data", 4));
    check_record("key1", "data");
    CHECK(fdb_insert(db, "key2", 4, "data", 4));
    check_record("key1", "data");
    CHECK(fdb_insert(db, "key3", 4, "data", 4));
    check_record("key1", "data");

    CHECK(fdb_save(db, "./fdb-serialize.bin"));
}

TEST(FdbSerializeGroup, Load)
{
    // clean db
    CHECK(fdb_load(db, "./fdb-serialize.bin"));
    // check records
}

