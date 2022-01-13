#include "../fs/operations.h"
#include <assert.h>
#include <string.h>

int main(){

    char *str = "abcdefghij";
    char buffer[40];

    char *path = "/f1";
    char *path2 = "f3";

    assert(tfs_init()!=-1);

    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd!=-1);

    assert(tfs_write(fd, str, strlen(str))==strlen(str));
    assert(tfs_close(fd)!=-1);

    fd = tfs_open(path, 0);
    assert(fd!=-1);
    ssize_t r = tfs_read(fd, buffer, sizeof(buffer)-1);
    assert(r==strlen(str));

    buffer[r]='\0';

    assert(strcmp(str, buffer)==0);

    assert(tfs_close(fd)!=-1);

    fd = tfs_open(path, TFS_O_APPEND);
    assert(fd!=-1);

    assert(tfs_write(fd, str, strlen(str))==strlen(str));
    assert(tfs_close(fd)!=-1);

    fd = tfs_open(path, 0);

    for (int i=0; i!=2; i++){
        r = tfs_read(fd, buffer, strlen(str));
        assert(r==strlen(str));
        buffer[r]='\0';
        assert(strcmp(str,buffer)==0);
        assert(tfs_copy_to_external_fs(path, path2) != -1);
    }
    assert(tfs_close(fd)!=-1);

    printf("Successful test.\n");

    return 0;

}