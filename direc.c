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


DIR *dir;
struct dirent *de;
struct stat statbuff;

struct node {
    struct node *next;
    char* direc_name;
};
typedef struct node node;

node* new_Node(char *name) {
    node* result = (node*) malloc(sizeof(node));
    result->direc_name = name;
    result->next = NULL; 
    return result;
}

struct direc_queue{
    node *head;
    int size;
};
typedef struct direc_queue direc_queue;

direc_queue* init_q() {
    direc_queue *q = (direc_queue*) malloc(sizeof(direc_queue));
    q->head = NULL;
    q->size = 0; 
    return q;
}

direc_queue *directories;     // global direc_queue




void enqueue(char* direc_name, direc_queue *q) {
    node* temp = new_Node(direc_name);
    q->size++;
    if(q->head == NULL) {
        q->head = temp;
        return;
    }
    temp->next = q->head;
    q->head = temp;
    return;
}


char* dequeue(direc_queue *q) {
    if (q->head == NULL) return NULL;  // queue is empty

    node* temp = q->head;

    if (q->size != 1) {     // if queue has more than 1 item, then update head
        temp = q->head;
        q->head = q->head->next;
    }
    q->size--;
    char* result = temp->direc_name;
    free(temp);
    return result;
}


void delete_q(direc_queue *q) {
    while (q->size > 0) {
        char *temp = dequeue(q);
        free(temp);
    }
    return;
}

void print_q(direc_queue *q) {
    if (q->size == 0) {
        if(DEBUG) printf("Queue is empty.\n");
        return;
    }
    node* ptr = q->head;
    int i = 0;
    while (i < q->size) {
        printf("%s\n",ptr->direc_name);
        ptr = ptr->next;
        i++;
    }
}





void* direc_worker(void* args) {
    return NULL;
}

void direc_traversal(char* path) {
    dir = opendir(path);
    de = readdir(dir);
    chdir(path);
    while (de != NULL) {
        int x = stat(de->d_name,&statbuff);
        if (x == -1) perror(de->d_name);
        if (S_ISDIR(statbuff.st_mode) && de->d_name[0] != '.') {
            char *file = de->d_name;
            int flen = strlen(file);
            int plen = strlen(path);
            char* newpath = malloc(plen+flen+2);
            memcpy(newpath,path,plen);
            newpath[plen] = '/';
            memcpy(newpath+plen+1,file,flen+1);
            enqueue(newpath,directories);
        }
        de = readdir(dir);
    }
    closedir(dir);
    free(path);
    return;
}





int main(int argc, char** argv)
{
    directories = init_q();
    char* path = getcwd(NULL,200);
    printf("%s\n",path); 

    
    direc_traversal(path);
    int i = 0;
    while (directories->size > 0) {
        if (directories->size == 1) printf("%s\n",directories->head->direc_name);
        direc_traversal(dequeue(directories));
        i++;
    }

    printf("i: %d\n",i);

    return 0;
}