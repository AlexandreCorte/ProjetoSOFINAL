#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>

#define SIZE 10
#define NUMBER_OF_THREADS 10
#define OUTPUT_SIZE 150

struct thread_info{
    int fd_towrite;
    char *input;
};

void* thread_func(void *arg){
    struct thread_info *info = arg;
    int fd = info->fd_towrite;
    char* input_to_write = info->input;
    assert(tfs_write(fd, input_to_write, SIZE)!=-1);
    return 0;
}

int main() {
    char *path = "/f10";

    struct thread_info *arg[10];

    pthread_t thread[10];

    char input1[SIZE];
    char input2[SIZE];
    char input3[SIZE];
    char input4[SIZE];
    char input5[SIZE];
    char input6[SIZE];
    char input7[SIZE];
    char input8[SIZE];
    char input9[SIZE];
    char input10[SIZE];
    memset(input1, 'A', SIZE);
    memset(input2, 'B', SIZE);
    memset(input3, 'C', SIZE);
    memset(input4, 'D', SIZE);
    memset(input5, 'E', SIZE);
    memset(input6, 'F', SIZE);
    memset(input7, 'G', SIZE);
    memset(input8, 'H', SIZE);
    memset(input9, 'I', SIZE);
    memset(input10,'J', SIZE);

    for (int i=0; i<10; i++){
        arg[i] = calloc(1, sizeof(struct thread_info));
    }

    char output[OUTPUT_SIZE];

    assert(tfs_init() != -1);

    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd!=-1);

    for (int i=0; i<10; i++){
        arg[i]->fd_towrite=fd;
    }
    arg[0]->input=input1;
    arg[1]->input=input2;
    arg[2]->input=input3;
    arg[3]->input=input4;
    arg[4]->input=input5;
    arg[5]->input=input6;
    arg[6]->input=input7;
    arg[7]->input=input8;
    arg[8]->input=input9;
    arg[9]->input=input10;

    for (int tnumber=0; tnumber<NUMBER_OF_THREADS; tnumber++){
        assert(pthread_create(&thread[tnumber], NULL, thread_func, arg[tnumber])!=-1);
    }

    for (int tnumber=0; tnumber<NUMBER_OF_THREADS; tnumber++){
        assert(pthread_join(thread[tnumber], NULL)!=-1);
    }

    assert(tfs_close(fd)!=-1);

    fd = tfs_open(path, 0);
    assert(fd!=-1);

    assert(tfs_read(fd, output, sizeof(output)-1)!=-1);

    for (int i=0; i!=strlen(output); i++){
        putc(output[i], stdout);
    }

    printf("\n");
    assert(strlen(output)==100);
    assert(tfs_close(fd)!=-1);
    assert(tfs_destroy()!=-1);
    printf("Successful test.\n");
    return 0;
}