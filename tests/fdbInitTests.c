/*
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 */
#include "fdb_private.h"
#include "fdb/fdb.h"
#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>

Test(fdb_init, already_deinitialized)
{
    fdb db = NULL;

    db = fdb_init("foo");
    fdb_deinit(db);
    /* TODO: How to detect if a data-base was de-initialized, freed,
       but the user calls de-init on it again? */
    //fdb_deinit(db);
}

Test(fdb_init, init_pass)
{
    fdb db1 = NULL;
    fdb db2 = NULL;

    db1 = fdb_init("foo");
    cr_assert_not_null(db1);
    cr_assert_str_eq("foo", db1->name);
    cr_assert_neq(db1->id, 0);
    fdb_deinit(db1);

    db2 = fdb_init("bar");
    cr_assert_not_null(db2);
    cr_assert_str_eq("bar", db2->name);
    cr_assert_neq(db2->id, 0);
    fdb_deinit(db2);
}

Test(fdb_init, init_fail_with_null)
{
    fdb db = NULL;

    db = fdb_init(NULL);
    cr_assert_null(db);
}

Test(fdb_init, duplicate_name)
{
    fdb db = NULL;
    fdb db1 = NULL;

    db = fdb_init("foo");
    cr_assert_not_null(db);
    cr_assert_neq(db->id, 0);
    cr_assert_str_eq("foo", db->name);

    db1 = fdb_init("foo");
    cr_assert_neq(db1, db);
    cr_assert_neq(db->id, db1->id);
    cr_assert_str_eq("foo", db1->name);

    fdb_deinit(db);
    fdb_deinit(db1);
}
