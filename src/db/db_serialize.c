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
fdb_file_open(fdb db, const char* filename, const char* mode)
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
    db->file_on_disk->f = fopen(db->file_on_disk->name, mode);
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


bool dict_visit_save_cb(const void* key, void* data, void* user_data)
{
    fdb_file_t* ffile = (fdb_file_t *) user_data;
    fnode       node = (fnode) data;
    size_t      ret = 0;

    ret = fwrite(node, node->header.node_size, 1, ffile->f);
    if (ret != 1) {
        FDB_ERROR("Error writing key_type %"PRIu16
                  ", data_type %" PRIu16 
                  " write to file: %s", 
                  node->header.key_type, 
                  node->header.data_type,
                  ffile->name);
        return false;
    }

    return true;
}

bool
fdb_save_to_file(fdb db)
{
    size_t record_count;

    record_count = dict_traverse(db->dstore, &dict_visit_save_cb, db->file_on_disk);
    FDB_INFO("Wrote %lu records to %s.", record_count, db->file_on_disk->name);

    return (record_count > 0);
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

    if (!fdb_file_open(db, filename, "ab+")) {
        FDB_ERROR("Cannot open file %s.", filename);
        return false;
    }

    ret = fdb_save_to_file(db);
    fdb_file_close(db);

    return ret;
}

bool
fdb_load_from_file(fdb db)
{
    // assert no data in db
    fdb_file_t*         ffile = db->file_on_disk;
    bool                ret = false;
    fnode               n;
    struct node_header_ nh;

    // read node header 
    fread(&nh, sizeof(struct node_header_), 1, ffile->f);
    while(!feof(ffile->f)) {
        size_t len = nh.node_size - sizeof(struct node_header_);
        size_t nsize = sizeof(struct node_header_) + len;

        n = calloc(1, nsize);
        if (!n) {
            FDB_ERROR("Cannot allocate node : %zu", nsize);
            ret = false;
            break;
        }

        n->header.key_type = nh.key_type;
        n->header.key_size = nh.key_size;
        n->header.data_size = nh.data_type;
        n->header.data_size = nh.data_size;

        fread(n->content, len, 1, ffile->f);
        if (feof(ffile->f)) {
            FDB_ERROR("Partial node header found. Truncated file!");
            ret = false;
            break;
        }

        ret = fdb_insert(db,
                         fnode_get_key(n),
                         fnode_get_keysize(n),
                         fnode_get_data(n),
                         fnode_get_datasize(n));
        if (!ret) {
            FDB_ERROR("Cannot load record. "
                      "\nkey: %"PRIx8 "\ndata: %"PRIx8,
                      *((uint8_t*) fnode_get_key(n)),
                      *((uint8_t*) fnode_get_data(n)));
            break;
        }
    }

    return ret;
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

    if (!fdb_file_open(db, filename, "r")) {
        return false;
    }

    ret = fdb_load_from_file(db);
    fdb_file_close(db);

    return ret;
}