#include "../fs/operations.h"
#include <assert.h>
#include <string.h>

int main() {

    char *str = "AAA!";
    char *path = "/f1";
    char *path2 = "/f2";
    char buffer[40];

    assert(tfs_init() != -1);

    int f;
    ssize_t r;

    int w;
    ssize_t w1;

    f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    w = tfs_open(path2, TFS_O_CREAT);
    assert(w != -1);

    r = tfs_write(f, str, strlen(str));
    assert(r == strlen(str));

    int a = tfs_copy_to_external_fs(path, path2);
    assert(a!= -1);

    int z = tfs_read(w, buffer, sizeof(buffer)-1);
    assert(z == strlen(str));

    printf("Successful test.\n");

    return 0;
}