#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>

int main(){
    char *path="/f5";

    tfs_init();

    assert(tfs_open(path, TFS_O_CREAT)!=-1);

    printf("Successful Test.\n");
    return 0;
}