/*
 * Copyright © 2016 Shivanand Velmurugan. All Rights Reserved.
 *
 * Persistence — save/load fdb contents to a flat binary file.
 *
 * On-disk record format (little-endian):
 *
 *   [ uint32_t key_size ][ uint32_t val_size ][ key bytes ][ val bytes ]
 *
 * Records are written sequentially with no header or checksum.
 */

#define _POSIX_C_SOURCE 200809L

#include "fdb_private.h"
#include "fdb_log.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---------------------------------------------------------------------- */
/* fdb_file_s — wraps a FILE pointer with its name (for error messages)  */
/* ---------------------------------------------------------------------- */

struct fdb_file_s {
    FILE* f;
    char* name;
};

static bool
fdb_file_open(fdb db, const char* filename, const char* mode)
{
    assert(db);
    assert(filename);

    db->file_on_disk = malloc(sizeof(struct fdb_file_s));
    if (!db->file_on_disk) return false;

    db->file_on_disk->name = strdup(filename);
    if (!db->file_on_disk->name) {
        free(db->file_on_disk);
        db->file_on_disk = NULL;
        return false;
    }

    db->file_on_disk->f = fopen(db->file_on_disk->name, mode);
    if (!db->file_on_disk->f) {
        free(db->file_on_disk->name);
        free(db->file_on_disk);
        db->file_on_disk = NULL;
        return false;
    }

    return true;
}

static void
fdb_file_close(fdb db)
{
    if (!db->file_on_disk) return;

    if (db->file_on_disk->f) {
        fclose(db->file_on_disk->f);
        db->file_on_disk->f = NULL;
    }
    free(db->file_on_disk->name);
    free(db->file_on_disk);
    db->file_on_disk = NULL;
}

/* ---------------------------------------------------------------------- */
/* Save                                                                   */
/* ---------------------------------------------------------------------- */

static bool
save_visitor(fdb_node_t node, void* userdata)
{
    fdb_file_t* ff = (fdb_file_t*)userdata;
    uint32_t    ksz = (uint32_t)node.key_size;
    uint32_t    vsz = (uint32_t)node.data_size;

    if (fwrite(&ksz, sizeof(ksz), 1, ff->f) != 1 ||
        fwrite(&vsz, sizeof(vsz), 1, ff->f) != 1 ||
        fwrite(node.key,  ksz, 1, ff->f) != 1     ||
        fwrite(node.data, vsz, 1, ff->f) != 1)
    {
        FDB_ERROR("Write error on file: %s", ff->name);
        return false;
    }
    return true;
}

bool
fdb_save(fdb db, const char* filename)
{
    if (!db || !filename) return false;

    if (!fdb_file_open(db, filename, "wb")) {
        FDB_ERROR("Cannot open file for writing: %s", filename);
        return false;
    }

    size_t count = db->ops->traverse(db->storage_ctx,
                                     save_visitor,
                                     db->file_on_disk);
    FDB_INFO("Wrote %zu records to %s.", count, db->file_on_disk->name);
    fdb_file_close(db);
    return count > 0;
}

/* ---------------------------------------------------------------------- */
/* Load                                                                   */
/* ---------------------------------------------------------------------- */

bool
fdb_load(fdb db, const char* filename)
{
    if (!db || !filename) return false;

    if (!fdb_file_open(db, filename, "rb")) {
        FDB_ERROR("Cannot open file for reading: %s", filename);
        return false;
    }

    FILE* f     = db->file_on_disk->f;
    bool  ok    = true;
    int   count = 0;

    for (;;) {
        uint32_t ksz, vsz;

        if (fread(&ksz, sizeof(ksz), 1, f) != 1) break; /* EOF or error */
        if (fread(&vsz, sizeof(vsz), 1, f) != 1) {
            FDB_ERROR("Truncated record header in %s", filename);
            ok = false;
            break;
        }

        void* key = malloc(ksz);
        void* val = malloc(vsz);
        if (!key || !val) {
            FDB_ERROR("Out of memory loading record");
            free(key);
            free(val);
            ok = false;
            break;
        }

        if (fread(key, ksz, 1, f) != 1 || fread(val, vsz, 1, f) != 1) {
            FDB_ERROR("Truncated record body in %s", filename);
            free(key);
            free(val);
            ok = false;
            break;
        }

        ok = fdb_insert(db, key, ksz, val, vsz);
        free(key);
        free(val);
        if (!ok) {
            FDB_ERROR("Failed to insert record %d from %s", count, filename);
            break;
        }
        count++;
    }

    FDB_INFO("Loaded %d records from %s.", count, filename);
    fdb_file_close(db);
    return ok && count > 0;
}
