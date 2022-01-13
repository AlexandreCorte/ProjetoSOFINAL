#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tfs_init() {
    state_init();
    mutex_init(&open_lock);
    mutex_init(&inode_create_lock);
    mutex_init(&add_dir_mutex);
    mutex_init(&open_table_lock);
    mutex_init(&data_alloc_mutex);
    /* create root inode */
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }
    return 0;
}

int tfs_destroy() {
    state_destroy();
    mutex_destroy(&open_lock);
    mutex_destroy(&inode_create_lock);
    mutex_destroy(&add_dir_mutex);
    mutex_destroy(&open_table_lock);
    mutex_destroy(&data_alloc_mutex);
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

int tfs_lookup(char const *name) {
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(ROOT_DIR_INUM, name);
}

int tfs_open(char const *name, int flags) {
    int inum;
    size_t offset;

    /* Checks if the path name is valid */
    if (!valid_pathname(name)) {
        return -1;
    }
    mutex_lock(&open_lock);
    inum = tfs_lookup(name);
    if (inum >= 0) {
        /* The file already exists */
        inode_t *inode = inode_get(inum);
        if (inode == NULL) {
            mutex_unlock(&open_lock);
            return -1;
        }

        /* Trucate (if requested) */
        if (flags & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                /*write lock no inode*/
                size_t blocks_to_free = inode->i_size / BLOCK_SIZE;
                for (int i = 0; i <= blocks_to_free; i++) {
                    if (i >= 10) {
                        int *block = data_block_get(inode->i_data_block[10]);
                        data_block_free(block[i - 10]);
                    }
                    if (data_block_free(inode->i_data_block[i]) == -1) {
                        mutex_unlock(&open_lock);
                        return -1;
                    }
                    if (i == blocks_to_free && blocks_to_free >= 10) {
                        data_block_free(inode->i_data_block[10]);
                    }
                    inode->i_size = 0;
                }
            }
            mutex_unlock(&open_lock);
        }
        /* Determine initial offset */
        if (flags & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
        mutex_unlock(&open_lock);
    } else if (flags & TFS_O_CREAT) {
        /* The file doesn't exist; the flags specify that it should be
         * created*/
        /* Create inode */
        /*MUTEX LOCK*/
        inum = inode_create(T_FILE);
        if (inum == -1) {
            mutex_unlock(&open_lock);
            return -1;
        }
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            mutex_unlock(&open_lock);
            return -1;
        }
        mutex_unlock(&open_lock);
        offset = 0;
    } else {
        mutex_unlock(&open_lock);
        return -1;
    }

    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    return add_to_open_file_table(inum, offset, flags);

    /* Note: for simplification, if file was created with TFS_O_CREAT and
     * there is an error adding an entry to the open file table, the file is
     * not opened but it remains created */
}

int tfs_close(int fhandle) { return remove_from_open_file_table(fhandle); }

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    size_t bytes_written = 0;
    size_t written = to_write;
    open_file_entry_t *file = get_open_file_entry(fhandle);
    mutex_lock(&file->lock);
    if (file == NULL) {
        mutex_unlock(&file->lock);
        return -1;
    }
    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    rwlock_write_lock(&inode->lock);
    if (inode == NULL) {
        mutex_unlock(&file->lock);
        rwlock_unlock(&inode->lock);
        return -1;
    }

    if (file->flags & TFS_O_APPEND) {
        file->of_offset = inode->i_size;
    }

    if (file->flags & TFS_O_TRUNC) {
        if (inode->i_size > 0) {
            /*write lock no inode*/
            size_t blocks_to_free = inode->i_size / BLOCK_SIZE;
            for (int i = 0; i <= blocks_to_free; i++) {
                if (i >= 10) {
                    int *block = data_block_get(inode->i_data_block[10]);
                    data_block_free(block[i - 10]);
                }
                if (data_block_free(inode->i_data_block[i]) == -1) {
                    mutex_unlock(&file->lock);
                    rwlock_unlock(&inode->lock);
                    return -1;
                }
                if (i == blocks_to_free && blocks_to_free >= 10) {
                    data_block_free(inode->i_data_block[10]);
                }
                inode->i_size = 0;
            }
        }
        file->of_offset = 0;
    }

    if (to_write > 0) {
        if (inode->i_size == 0) {
            inode->i_data_block[0] = data_block_alloc();
        }

        while (to_write > 0) {

            size_t block_index = file->of_offset / BLOCK_SIZE;
            size_t block_offset = file->of_offset % BLOCK_SIZE;

            if (block_index >= 10) {
                int *block = data_block_get(inode->i_data_block[10]);
                if (block == NULL) {
                    mutex_unlock(&file->lock);
                    rwlock_unlock(&inode->lock);
                    return -1;
                }

                size_t ref_block_index = (file->of_offset / BLOCK_SIZE) - 10;

                void *ref_block = data_block_get(block[ref_block_index]);

                if (to_write + block_offset < BLOCK_SIZE) {

                    memcpy(ref_block + (int)block_offset,
                           buffer + bytes_written, to_write);
                    file->of_offset += to_write;

                    if (file->of_offset > inode->i_size) {
                        inode->i_size = file->of_offset;
                    }

                    bytes_written += to_write;
                    to_write = 0;
                }

                else {
                    unsigned long write_first = BLOCK_SIZE - block_offset;

                    memcpy(ref_block + (int)block_offset,
                           buffer + bytes_written, write_first);

                    file->of_offset += write_first;

                    if (file->of_offset > inode->i_size) {
                        inode->i_size = file->of_offset;
                    }

                    bytes_written += write_first;
                    to_write -= write_first;

                    if (ref_block_index + 1 < (BLOCK_SIZE / sizeof(int))) {
                        block[ref_block_index + 1] = data_block_alloc();
                    }

                    else {
                        written = bytes_written;
                        to_write = 0;
                    }
                }
            }

            else {
                void *block = data_block_get(inode->i_data_block[block_index]);
                if (block == NULL) {
                    mutex_unlock(&file->lock);
                    rwlock_unlock(&inode->lock);
                    return -1;
                }

                if (to_write + block_offset < BLOCK_SIZE) {

                    /* Perform the actual write */
                    memcpy(block + block_offset, buffer + bytes_written,
                           to_write);

                    /* The offset associated with the file handle is
                     * incremented accordingly */
                    file->of_offset += to_write;
                    if (file->of_offset > inode->i_size) {
                        inode->i_size = file->of_offset;
                    }
                    bytes_written += to_write;
                    to_write = 0;

                } else {

                    size_t write_first = BLOCK_SIZE - block_offset;

                    /* Perform the actual write */
                    memcpy(block + block_offset, buffer + bytes_written,
                           write_first);

                    /* The offset associated with the file handle is
                     * incremented accordingly */
                    file->of_offset += write_first;
                    if (file->of_offset > inode->i_size) {
                        inode->i_size = file->of_offset;
                    }
                    bytes_written += write_first;
                    to_write -= write_first;

                    if (block_index + 1 < 10) {
                        inode->i_data_block[block_index + 1] =
                            data_block_alloc();
                    }

                    if (block_index + 1 == 10) {
                        inode->i_data_block[block_index + 1] =
                            data_block_alloc();
                        int *new_block = data_block_get(
                            inode->i_data_block[block_index + 1]);
                        new_block[0] = data_block_alloc();
                    }
                }
            }
        }
    }
    mutex_unlock(&file->lock);
    rwlock_unlock(&inode->lock);
    return (ssize_t)written;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    mutex_lock(&file->lock);
    if (file == NULL) {
        mutex_unlock(&file->lock);
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    rwlock_read_lock(&inode->lock);
    if (inode == NULL) {
        mutex_unlock(&file->lock);
        rwlock_unlock(&inode->lock);
        return -1;
    }

    /* Determine how many bytes to read */

    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    size_t bytes_read = to_read;
    size_t bytes_already_read = 0;

    while (to_read != 0) {

        size_t block_index = file->of_offset / BLOCK_SIZE;
        size_t block_offset = file->of_offset % BLOCK_SIZE;

        if (block_index >= 10) {

            int *block = data_block_get(inode->i_data_block[10]);
            if (block == NULL){
                mutex_unlock(&file->lock);
                rwlock_unlock(&inode->lock);
                return -1;
            }
            size_t ref_block_index = file->of_offset / BLOCK_SIZE - 10;

            void *ref_block = data_block_get(block[ref_block_index]);

            if (to_read + block_offset < BLOCK_SIZE) {
                memcpy(buffer + bytes_already_read,
                       ref_block + (int)block_offset, to_read);
                file->of_offset += to_read;
                bytes_already_read += to_read;
                to_read = 0;
            }

            else {
                size_t read_first = BLOCK_SIZE - block_offset;

                if (ref_block_index < (BLOCK_SIZE / sizeof(int))) {
                    memcpy(buffer + bytes_already_read,
                           ref_block + (int)block_offset, read_first);
                    file->of_offset += read_first;
                    bytes_already_read += read_first;
                    to_read -= read_first;
                } else {
                    mutex_unlock(&file->lock);
                    rwlock_unlock(&inode->lock);
                    return -1;
                }
            }
        }

        else {

            void *block = data_block_get(inode->i_data_block[block_index]);

            if (block == NULL){
                mutex_unlock(&file->lock);
                rwlock_unlock(&inode->lock);
                return -1;
            }

            if (to_read + block_offset < BLOCK_SIZE) {

                memcpy(buffer + bytes_already_read, block + block_offset,
                       to_read);
                file->of_offset += to_read;
                bytes_already_read += to_read;
                to_read = 0;
            }

            else {
                size_t read_first = BLOCK_SIZE - block_offset;

                memcpy(buffer + bytes_already_read, block + block_offset,
                       read_first);
                file->of_offset += read_first;
                bytes_already_read += read_first;
                to_read -= read_first;
            }
        }
    }
    mutex_unlock(&file->lock);
    rwlock_unlock(&inode->lock);
    return (ssize_t)bytes_read;
}

int tfs_copy_to_external_fs(char const *_source_path, char const *dest_path) {
    int f = tfs_open(_source_path, 0);
    if (f == -1)
        return -1;
    open_file_entry_t *file = get_open_file_entry(f);
    inode_t *inode = inode_get(file->of_inumber);
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    FILE *w = fopen(dest_path, "w");
    if (w == NULL)
        return -1;
    ssize_t bytes_read = 0;
    ssize_t bytes_processed = 0;
    while (bytes_processed != inode->i_size) {
        bytes_read = tfs_read(f, buffer, (size_t)(sizeof(buffer)));
        fwrite(buffer, sizeof(char), (size_t)bytes_read, w);
        memset(buffer, 0, sizeof(buffer));
        bytes_processed += bytes_read;
    }
    tfs_close(f);
    fclose(w);
    return 0;
}
