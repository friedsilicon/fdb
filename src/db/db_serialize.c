#define _POSIX_C_SOURCE 200809L

#include "fdb_private.h"
#include "fdb_log.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

struct fdb_file_s
{
    FILE* f;
    char* name;
};

static bool
fdb_file_open(fdb db, const char* filename)
{
    assert(db);
    assert(filename);

    db->file_on_disk = malloc(sizeof(struct fdb_file_s));
    if (!db->file_on_disk) {
        // out of memory
        return false;
    }

    db->file_on_disk->name = strdup(filename);
    if (!db->file_on_disk->name) {
        // out of memory
        return false;
    }

    // Open binary file for read/append
    db->file_on_disk->f = fopen(db->file_on_disk->name, "ab+");
    if (!db->file_on_disk->f) {
        return false;
    }

    return true;
}

void
fdb_file_close(fdb db)
{
    if (db->file_on_disk->f) {
        fclose(db->file_on_disk->f);
        db->file_on_disk->f = NULL;
    }

    if (db->file_on_disk->name) {
        free(db->file_on_disk->name);
        db->file_on_disk->name = NULL;
    }

    if (db->file_on_disk) {
        free(db->file_on_disk);
        db->file_on_disk = NULL;
    }
}

bool
fdb_save_to_file(fdb db)
{

    return true;
}

bool
fdb_save(fdb db, const char* filename)
{
    bool ret = false;

    if (!db || !filename) {
        return false;
    }

    // cannot save if there are any active txn
    // cannot save if there are operations

    if (!fdb_file_open(db, filename)) {
        return false;
    }

    ret = fdb_save_to_file(db);
    fdb_file_close(db);

    return ret;
}

bool
fdb_load_from_file(fdb db)
{
    return true;
}

bool
fdb_load(fdb db, const char* filename)
{
    bool ret = false;
    if (!db || !filename) {
        return false;
    }

    // cannot load if there are any active txn
    // cannot load if there are operations
    // cannot load if there is already data in the db?

    if (!fdb_file_open(db, filename)) {
        return false;
    }

    ret = fdb_load_from_file(db);
    fdb_file_close(db);


    return true;
}