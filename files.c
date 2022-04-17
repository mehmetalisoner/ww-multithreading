#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#define DEBUG 1
#define Q_SIZE 500

DIR* dir;
struct dirent* de;
struct stat statbuff;


struct file_queue{
    char* files[Q_SIZE];
    int start, stop;
    int full;
};
typedef struct file_queue file_queue;

file_queue* init_file_q(){
    file_queue *q = malloc(sizeof(file_queue));
    q->start = q->stop = 0;
    q->full = 0;
    return q;
}

file_queue *files;      // global file queue




void enqueue_file(char *file_name, file_queue *q) {
    q->files[q->stop] = file_name;
    q->stop++;
    if (q->stop == Q_SIZE) q->stop = 0;
    if (q->start == q->stop) q->full = 1;
}

char* dequeue_file(file_queue *q) {
    char* file_name;
    file_name = q->files[q->start];
    q->start++;
    if (q->start == Q_SIZE) q->start = 0;
    q->full = 0;
    return file_name;
}


void print_files(file_queue *q) {
    while(q->start != q->stop) {
        printf("%s\n", q->files[q->start]);
        q->start++;
    }
}


void direc_traversal(char* path) {
    dir = opendir(path);
    de = readdir(dir);
    chdir(path);
    while (de != NULL) {
        int x = stat(de->d_name,&statbuff);
        if (x == -1) perror(de->d_name);
        if (S_ISREG(statbuff.st_mode) && de->d_name[0] != '.') {
            char *file = de->d_name;
            int flen = strlen(file);
            int plen = strlen(path);
            char* newpath = malloc(plen+flen+2);
            memcpy(newpath,path,plen);
            newpath[plen] = '/';
            memcpy(newpath+plen+1,file,flen+1);
            enqueue_file(newpath,files);
        }
        de = readdir(dir);
    }
    closedir(dir);
    free(path);
    return;
}



int main(int argc, char** argv) 
{
    files = init_file_q();

    char* path = getcwd(NULL,200);
    printf("%s\n",path); 

    
    direc_traversal(path);
    int i = 0;
    // while (files->start < files->stop) {
    //     direc_traversal(dequeue_file(files));
    //     files->start++;
    //     i++;
    // }

    // printf("i: %d\n",i);

    print_files(files);

    return EXIT_SUCCESS;
}