#include "../fs/operations.h"
#include <assert.h>
#include <string.h>

#define SIZE 5
#define OUTPUT 11

int main(){
    char *path = "/f1";
    char *path2 = "f2";

    char buffer[SIZE] = "abcde";
    char second_buffer[SIZE] = "fghij";
    char output[OUTPUT];
    char *expected_output = "abcdefghij";

    assert(tfs_init() != -1);

    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);

    assert(tfs_write(fd, buffer, SIZE)==SIZE);
    assert(tfs_write(fd, second_buffer, SIZE)==SIZE);

    assert(tfs_close(fd) != -1);

    fd = tfs_open(path, 0);
    assert(fd != -1);

    ssize_t r = tfs_read(fd, output, sizeof(output) - 1);
    assert(r == strlen(expected_output));

    output[r] = '\0';

    assert(strcmp(output, expected_output) ==0);

    assert(tfs_copy_to_external_fs(path, path2) != -1);

    assert(tfs_close(fd) != -1);

    printf("Successful test.\n");

    return 0;
    
}