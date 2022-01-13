#ifndef STATE_H
#define STATE_H

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>

/*
 * Directory entry
 */
typedef struct {
    char d_name[MAX_FILE_NAME];
    int d_inumber;
} dir_entry_t;

typedef enum { T_FILE, T_DIRECTORY } inode_type;

/*
 * I-node
 */
typedef struct {
    inode_type i_node_type;
    size_t i_size;
    int i_data_block[11];
    pthread_rwlock_t lock;
    /* in a real FS, more fields would exist here */
} inode_t;

typedef enum { FREE = 0, TAKEN = 1 } allocation_state_t;

/*
 * Open file entry (in open file table)
 */
typedef struct {
    int of_inumber;
    size_t of_offset;
    int flags;
    pthread_mutex_t lock;
} open_file_entry_t;

#define MAX_DIR_ENTRIES (BLOCK_SIZE / sizeof(dir_entry_t))

/*Mutex*/
pthread_mutex_t open_lock;
pthread_mutex_t inode_create_lock;
pthread_mutex_t open_table_lock;
pthread_mutex_t add_dir_mutex;
pthread_mutex_t data_alloc_mutex;

void state_init();
void state_destroy();

int mutex_init(pthread_mutex_t* lock);
int mutex_lock(pthread_mutex_t* lock);
int mutex_unlock(pthread_mutex_t* lock);
int mutex_destroy(pthread_mutex_t* lock);
int rwlock_initialize(pthread_rwlock_t* lock);
int rwlock_destroy(pthread_rwlock_t* lock);
int rwlock_write_lock(pthread_rwlock_t* lock);
int rwlock_read_lock(pthread_rwlock_t* lock);
int rwlock_unlock(pthread_rwlock_t* lock);

int inode_create(inode_type n_type);
int inode_delete(int inumber);
inode_t *inode_get(int inumber);

int clear_dir_entry(int inumber, int sub_inumber);
int add_dir_entry(int inumber, int sub_inumber, char const *sub_name);
int find_in_dir(int inumber, char const *sub_name);

int data_block_alloc();
int data_block_free(int block_number);
void *data_block_get(int block_number);

int add_to_open_file_table(int inumber, size_t offset, int flags);
int remove_from_open_file_table(int fhandle);
open_file_entry_t *get_open_file_entry(int fhandle);

#endif // STATE_H
