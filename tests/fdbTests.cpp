/*
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#include "fdb_private.h"
#include "CppUTest/TestHarness.h"
#include "fdb/fdb.h"
#include <stdio.h>
#include <string.h>

// clang-format off
TEST_GROUP(FdbInitGroup){};

TEST_GROUP(FdbOpsGroup)
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

TEST(FdbInitGroup, AlreadyDeInitialized)
{
    fdb db = NULL;

    db = fdb_init("foo");
    fdb_deinit(&db);
    CHECK(db == NULL);
    fdb_deinit(&db);
}

TEST(FdbInitGroup, InitPass)
{
    fdb db1 = NULL;
    fdb db2 = NULL;

    db1 = fdb_init("foo");
    CHECK(db1 != NULL);
    STRCMP_EQUAL("foo", db1->name);
    CHECK(db1->id != 0);
    fdb_deinit(&db1);

    db2 = fdb_init("bar");
    CHECK(db2 != NULL);
    STRCMP_EQUAL("bar", db2->name);
    CHECK(db2->id != 0);
    fdb_deinit(&db2);
}

TEST(FdbInitGroup, InitFailWithNull)
{
    fdb db = NULL;

    db = fdb_init(NULL);
    CHECK(db == NULL);
}

TEST(FdbInitGroup, FdbInitWithDuplicateName) /* TODO: Should this be allowed? */
{
    fdb db = NULL;
    fdb db1 = NULL;

    db = fdb_init("foo");
    CHECK(db != NULL);
    CHECK(db->id != 0);
    STRCMP_EQUAL("foo", db->name);

    db1 = fdb_init("foo");
    CHECK(db1 != db);
    CHECK(db->id != db1->id);
    STRCMP_EQUAL("foo", db1->name);

    fdb_deinit(&db);
    fdb_deinit(&db1);
}

TEST(FdbOpsGroup, Insert)
{
    CHECK(fdb_insert(db, "key", 3, "data", 4));
    check_record("key", "data");

    CHECK_FALSE(fdb_insert(db, "key", 3, "data", 4));

    CHECK(fdb_insert(db, "key1", 4, "data", 4));
    check_record("key1", "data");
}

TEST(FdbOpsGroup, Exists)
{
    CHECK(fdb_insert(db, "key", 3, "data", 4));
    CHECK(fdb_exists(db, "key", 3));
    CHECK_FALSE(fdb_exists(db, "key1", 4));
}

TEST(FdbOpsGroup, Update)
{
    CHECK(fdb_insert(db, "key", 3, "data", 4));
    check_record("key", "data");

    CHECK(fdb_update(db, "key", 3, "data1", 5));
    check_record("key", "data1");
}

TEST(FdbOpsGroup, Remove)
{
    CHECK(fdb_insert(db, "key", 3, "data", 4));
    check_record("key", "data");

    CHECK(fdb_remove(db, "key", 3));
    CHECK_FALSE(fdb_exists(db, "key", 3));
}

TEST(FdbOpsGroup, Iterate)
{
    fiter iterator = NULL;
    fnode node = NULL;

    fdb_insert(db, "1", 1, "data1", 5);
    fdb_insert(db, "2", 1, "data2", 5);
    fdb_insert(db, "3", 1, "data3", 5);

    iterator = fdb_iterate(db);
    CHECK(iterator != NULL);

    CHECK(fiter_hasnext(iterator));
    node = fiter_next(iterator);
    CHECK(node != NULL);
    check_node(node, "1", 1, "data1", 5);

    CHECK(fiter_hasnext(iterator));
    node = fiter_next(iterator);
    CHECK(node != NULL);
    check_node(node, "2", 1, "data2", 5);

    CHECK(fiter_hasnext(iterator));
    node = fiter_next(iterator);
    CHECK(node != NULL);
    check_node(node, "3", 1, "data3", 5);

    CHECK(fiter_hasnext(iterator) == false);
    CHECK(fiter_next(iterator) == NULL);
}

TEST(FdbOpsGroup, Invalid)
{
    CHECK_FALSE(fdb_insert(db, NULL, 0, NULL, 0));
    CHECK_FALSE(fdb_insert(db, NULL, 0, "data", 4));
    CHECK_FALSE(fdb_insert(db, "key", 3, NULL, 0));
    CHECK_FALSE(fdb_insert(NULL, "key", 3, "data", 4));

    CHECK_FALSE(fdb_update(NULL, NULL, 0, NULL, 0));
    CHECK_FALSE(fdb_update(db, NULL, 0, NULL, 0));
    CHECK_FALSE(fdb_update(db, NULL, 0, "data", 4));
    CHECK_FALSE(fdb_update(db, "key", 3, NULL, 0));
    CHECK_FALSE(fdb_update(db, "key", 3, "data1", 5));
    CHECK_FALSE(fdb_update(NULL, "key", 3, "data1", 5));

    CHECK_FALSE(fdb_remove(NULL, NULL, 0));
    CHECK_FALSE(fdb_remove(db, NULL, 0));
    CHECK_FALSE(fdb_remove(db, "key", 3));
    CHECK_FALSE(fdb_remove(NULL, "key", 3));

    CHECK_FALSE(fdb_remove(db, NULL, 0));
    CHECK_FALSE(fdb_remove(db, "key", 3));
    CHECK_FALSE(fdb_remove(db, NULL, 3));
    CHECK_FALSE(fdb_remove(db, "key", 0));

    CHECK_FALSE(fdb_exists(NULL, "key", 3));
    CHECK_FALSE(fdb_exists(db, NULL, 0));
    CHECK_FALSE(fdb_exists(db, NULL, 3));
    CHECK_FALSE(fdb_exists(db, "key", 0));

    CHECK(fnode_get_key(NULL) == NULL);
    CHECK(fnode_get_keysize(NULL) == 0);

    CHECK(fnode_get_data(NULL) == NULL);
    CHECK(fnode_get_datasize(NULL) == 0);
}
