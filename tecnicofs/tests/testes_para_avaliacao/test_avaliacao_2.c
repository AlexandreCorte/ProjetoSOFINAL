#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>

#define SIZE 100
#define OUTPUT_SIZE 50
#define SIZE_TO_READ 10
#define NUMBER_OF_THREADS 10

char final_output[SIZE+1];

struct thread_info{
    int fd_read;
    char *output;
};

void* thread_func(void *arg){
    struct thread_info *info = arg;
    int fd = info->fd_read;
    char* output_to_read = info->output;
    assert(tfs_read(fd, output_to_read, 10)!=-1);
    output_to_read[10]='\0';
    assert(strlen(output_to_read)==10);
    for (int i=0; i!=10; i++)
        putc(output_to_read[i], stdout);
    printf("\n");
    strcat(final_output, output_to_read);
    return 0;
}

int main(){
    char *path = "/f9";

    struct thread_info *arg[10];

    pthread_t thread[10];

    char* input = "alepweoiqrtvwoaisndisadnlasjalsdnlsajdbsaodsbdoisboidbsfoubfidlfbladiufbadioufbidsufleuiabeiaubfeiub";

    assert(tfs_init()!=-1);

    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd!=-1);

    assert(tfs_write(fd, input, SIZE)!=-1);
    assert(tfs_close(fd)!=-1);

    char output1[OUTPUT_SIZE];
    char output2[OUTPUT_SIZE];
    char output3[OUTPUT_SIZE];
    char output4[OUTPUT_SIZE];
    char output5[OUTPUT_SIZE];
    char output6[OUTPUT_SIZE];
    char output7[OUTPUT_SIZE];
    char output8[OUTPUT_SIZE];
    char output9[OUTPUT_SIZE];
    char output10[OUTPUT_SIZE];

    int folder = tfs_open(path, 0);
    assert(folder!=-1);

    for (int i=0; i<10; i++){
        arg[i] = calloc(1, sizeof(struct thread_info));
    }

    arg[0]->output=output1;
    arg[1]->output=output2;
    arg[2]->output=output3;
    arg[3]->output=output4;
    arg[4]->output=output5;
    arg[5]->output=output6;
    arg[6]->output=output7;
    arg[7]->output=output8;
    arg[8]->output=output9;
    arg[9]->output=output10;

    for (int i=0; i<10; i++){
        arg[i]->fd_read=folder;
    }

    for (int tnumber=0; tnumber<NUMBER_OF_THREADS; tnumber++){
        assert(pthread_create(&thread[tnumber], NULL, thread_func, arg[tnumber])!=-1);
    }

    for (int tnumber=0; tnumber<NUMBER_OF_THREADS; tnumber++){
        assert(pthread_join(thread[tnumber], NULL)!=-1);
    }

    assert(tfs_close(folder)!=-1);
    assert(tfs_destroy()!=-1);

    assert(strncmp(final_output, input, SIZE)==0);

    printf("Successful test.\n");
    return 0;
}