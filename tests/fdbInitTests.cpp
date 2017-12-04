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
// clang-format on

TEST(FdbInitGroup, AlreadyDeInitialized)
{
    fdb db = NULL;

    db = fdb_init("foo");
    fdb_deinit(db);
    /* TODO: How to detect if a data-base was de-initialized, freed,
       but the user calls de-init on it again? */
    //fdb_deinit(db);
}

TEST(FdbInitGroup, InitPass)
{
    fdb db1 = NULL;
    fdb db2 = NULL;

    db1 = fdb_init("foo");
    CHECK(db1 != NULL);
    STRCMP_EQUAL("foo", db1->name);
    CHECK(db1->id != 0);
    fdb_deinit(db1);

    db2 = fdb_init("bar");
    CHECK(db2 != NULL);
    STRCMP_EQUAL("bar", db2->name);
    CHECK(db2->id != 0);
    fdb_deinit(db2);
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

    fdb_deinit(db);
    fdb_deinit(db1);
}
