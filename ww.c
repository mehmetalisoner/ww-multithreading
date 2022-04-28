#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <ctype.h>

#ifndef DEBUG
#define DEBUG 1
#endif

#ifndef DEBUG2
#define DEBUG2 1
#endif

#define BUFSIZE 3
#define SIZE 40



int fd_in, bytes, fd_out;                               // for reading, writing files
int file_width;                                         // this will be the width of the output file
char buf[BUFSIZE];                                      // our buffer
char* crnt;                                             // this will hold our leftover characters from the buffer
int crntlen;                                            // length of variable char* crnt
int buf_pos, buf_start;                                 // buf_pos = current position in buffer buf, buf_start = where we will start copying in to fd_out 
int line_len;                                           // keep track of how many chars are currently on the output file line 
int whitespace_count;                                   // keep track of whitespace
int newline_count;                                      // keep track of new lines
char last_char;                                         // keep track of last character
int success = 0;                                        // checks if program ran successfully. for handling "word > file_width" error
int write_success = 0;
struct stat statbuff;                                   // stat structure to check incoming pathname's type
DIR* dir;                                               // variables to handle directory inputs 
struct dirent* de; 
int M, N;                                               // M = number_of_direc_threads; N = number_of_file_threads




int check_input_digit(char* s)
{
    for (int i = 0; i < strlen(s); i++) {
        if(!isdigit(s[i])) {
            if (DEBUG) printf("Given input is not an integer. \n");
            return -1;
        }
    }
    return 0;
}

/**
 * @brief Helper function that checks if every write() call
 * was performed without any errors.
 * 
 * @param written: Amount of bytes written. return value from write()
 */
void check_write(int written) 
{
    if (written == -1) {
        if (DEBUG) printf("Problem occured while writing to fd_out. \n");
        write_success = -1;
        return;
    }
}



/**
 * @brief This function prints out a word from the given file argv[2]
 * if there is enough space for it on the current line. If the the length
 * of the current word exceeds the file width, then we print out the word
 * on a new line and reset line_len. 
 * @param fd_out: This is the file descriptor that we will be writing our
 * bytes to.
 */
void add_word(int fd_out)
{
    int written = 0;
    int word_len = buf_pos - buf_start;                             // get length of word
    if (word_len > file_width) {
        success = -1;
        written = write(fd_out,"\n",1);
        check_write(written);
    }
    if ((line_len + word_len) > file_width) {                   // word or the continuation of the word doesn't fit into the line                      
        written = write(fd_out,"\n",1);
        check_write(written);
        line_len = word_len + 1 + crntlen;
    }
    else {                                                          // we still have space on the line
        if (line_len == crntlen) {
            line_len += word_len + 1;                                   // update line_len and add the length of current word
        }
        else {
            write(fd_out," ",1);
            line_len += word_len + 1;                                   // update line_len and add the length of current word
        }
    }

    crnt = realloc(crnt, crntlen + word_len + 1);                   // realloc crnt so we can copy word into it
    memcpy(&crnt[crntlen],&buf[buf_start],word_len);                // copy word into crnt
    //crnt[crntlen + word_len] = ' ';                               // replace '\n' with a space
    crnt[crntlen+word_len] = '\0';                                  // add null character 
    written = write(fd_out,crnt,crntlen+word_len);
    check_write(written);
    free(crnt);
    crntlen = 0;
    crnt = NULL;
    buf_start = buf_pos + 1;                                        // update buf_start

    if (success == -1) {
        printf("\n");                                               // print another new line if word is too long
        line_len = 0;
    }
}

/**
 * @brief This function prints out a paragraph if we have 
 * met the right conditions for it. It will first print out
 * whatever is left in crnt and then print 2 newline characters.
 * 
 * @param fd_out: This is the place we are going to be writing our
 * paragraphs into.
 */
void add_paragraph(int fd_out)
{
    int written = 0;
    if(crnt != NULL){
        written = write(fd_out, &crnt[0],crntlen);
        check_write(written);
    }
    written = write(fd_out,"\n\n",2);
    check_write(written);
    free(crnt);
    line_len = 0;
    crnt = NULL;
    crntlen = 0;
    newline_count = 0;
}

/**
 * @brief This function reads from the set file descriptor fd into the 
 * buffer buff. From there, we iterate through each buffer buff we get
 * and print out/write words through the add_word() and add_paragraph()
 * functions.
 * 
 * @param fd_in: This is the file descriptor from which we will be going
 * to read our bytes from.
 * 
 * @param fd_out: This is the file descriptor to which we will be going
 * to write our bytes to by calling add_word(fd_out) and add_paragraph(fd_out)
 */
void word_wrap(int fd_in, int fd_out)
{
    crnt = NULL;
    crntlen = 0;
    line_len = 0;

    // This is for reading from a file and printing out words
    while ((bytes = read(fd_in,buf,BUFSIZE)) > 0) {                     // iterate through given file
        buf_start = 0;
        for (buf_pos = 0; buf_pos < bytes; buf_pos++) {                 // iterate through buffer
            //if (buf[buf_pos] == ' ' || buf[buf_pos] == '\n') {        // current char is a newline or a whitespace
            if (isspace((int)buf[buf_pos]) != 0){                       // current character is whitespace
                if(buf[buf_pos] == ' ') whitespace_count++;             // update counters
                if(buf[buf_pos] == '\n') newline_count++;
                if ((buf_pos == 0 && crntlen == 0) || (last_char == ' ' || last_char == '\n')) {        // check for last character. update buf_start depending on it
                    last_char = buf[buf_pos];
                    buf_start = buf_pos + 1;
                }
                else {                                                  // if last_char wasn't a new line or a whitespace, then add the word
                    add_word(fd_out);
                }
            }
            else {                                                      // current char is anything but a newline or a whitespace
                if (last_char == ' ' || last_char == '\n') {            // check last character
                    if (newline_count >= 2) add_paragraph(fd_out);      // check how many new lines we saw till now     
                }
                else {
                    newline_count = 0;
                    whitespace_count = 0;
                }
                
            }
            last_char = buf[buf_pos];
        }
        if (buf_start < buf_pos) {                                      // if we have characters leftover in the buffer
            int add_on = buf_pos - buf_start;                           // get the leftover's length
            crnt = realloc(crnt,crntlen+add_on+1);
            memcpy(&crnt[crntlen],&buf[buf_start], add_on);
            crnt[crntlen+add_on] = '\0';
            crntlen += add_on;                                          // update crntlen
            line_len += add_on;                                         // add what was left to line_len
        }
    }
    buf_pos = buf_start = 0;
    if (crntlen != 0) add_word(fd_out);                                 // if we still have something left in crnt
    int temp = write(fd_out,"\n",1);                                    // end of file with new line
    if (temp == -1) return;
    return;
}




char* string_maker(char* path, char*file) 
{
    int plen = strlen(path);
    int flen = strlen(file);
    char* newpath = malloc(plen+flen+2);
    memcpy(newpath,path,plen);
    newpath[plen] = '/';
    memcpy(newpath+plen+1,file,flen+1);
    return newpath;
}


// DIRECTORY-STUFF:
// ---------------

struct direc_node {
    struct direc_node *next;
    char* direc_name;
};
typedef struct direc_node direc_node;

direc_node* new_direc_Node(char *name) {
    direc_node* result = (direc_node*) malloc(sizeof(direc_node));
    result->direc_name = name;
    result->next = NULL; 
    return result;
}

struct direc_queue{
    direc_node *head;
    int size, workers;
    pthread_mutex_t lock;
    pthread_cond_t dequeue_ready;
};
typedef struct direc_queue direc_queue;


direc_queue* init_direc_q() {
    direc_queue *q = (direc_queue*) malloc(sizeof(direc_queue));
    q->head = NULL;
    q->size = q->workers = 0; 
    pthread_mutex_init(&q->lock,NULL);
    pthread_cond_init(&q->dequeue_ready,NULL);
    return q;
}

void enqueue_direc(char* direc_name, direc_queue *q) {
    pthread_mutex_lock(&q->lock);

    pthread_t self = pthread_self();
    if(DEBUG2) printf("Direc_Thread: %ld enquing_direc(). %s\n",self,direc_name);

    direc_node* temp = new_direc_Node(direc_name);
    if (q->size == 0) {      // if Queue is empty
        q->head = temp;
        q->size++;
        pthread_mutex_unlock(&q->lock);
        pthread_cond_broadcast(&q->dequeue_ready);
        return;
    }
    q->size++;
    temp->next = q->head;
    q->head = temp;

    pthread_mutex_unlock(&q->lock);
    pthread_cond_broadcast(&q->dequeue_ready);

    return;
}

char* dequeue_direc(direc_queue *q) {
    pthread_mutex_lock(&q->lock);

    char* result = NULL;                    // output value
    pthread_t t = pthread_self();
    if(DEBUG) printf("Direc_Thread: %ld in dq() direc-q.\n",t);

    while (q->workers != 0) {               // check if there are any workers
        while (q->size == 0) {              // wait for items
            printf("Direc_Thread: %ld waiting in dq() direc-q.\n",t);
            q->workers--;                   // thread is waiting so it's not working
            if (q->workers == 0) break;
            pthread_cond_wait(&q->dequeue_ready, &q->lock);
        }
        if (q->head == NULL) return result; // sanity check for the last thread that might try to dequeue a NULL node

        direc_node* temp = q->head;         // dequeuing stuff
        q->head = q->head->next;
        q->size--;
        result = temp->direc_name;
        free(temp);
        break;
    }

    pthread_mutex_unlock(&q->lock);
    return result;
}

void delete_direc_q(direc_queue *q) {
    while (q->head != NULL) {
        direc_node * ptr = q->head;
        q->head = q->head->next;
        free(ptr);
    }
    return;
}

void print_direc_q(direc_queue *q) {
    if (q->size == 0) {
        if(DEBUG) printf("Queue is empty.\n");
        return;
    }
    direc_node* ptr = q->head;
    int i = 0;
    while (i < q->size) {
        printf("%s\n",ptr->direc_name);
        ptr = ptr->next;
        i++;
    }
}

// -----------------

// FILE STUFF:
// -----------  

typedef struct file_node {
    char* path;
    char* file;
}file_node;

typedef struct file_q{
    file_node* files[SIZE];
    int start, stop;
    int isFull;
    pthread_mutex_t lock;
    pthread_cond_t dequeue_ready, enqueue_ready;
    int workers;
}file_q;

// GLOBAL Qs
// ---------
file_q *files;              // global file queue
direc_queue *directories;   // global directory queue


file_q* init_file_q(){
    file_q *q = malloc(sizeof(file_q));
    q->start = q->stop = q->isFull = q->workers = 0;
    pthread_mutex_init(&q->lock,NULL);
    pthread_cond_init(&q->dequeue_ready,NULL);
    pthread_cond_init(&q->enqueue_ready,NULL);
    return q;
}

int is_files_empty(file_q *q) {
    if((q->start == q->stop) && (!q->isFull)) return 0;    // 0 = isEmpty
    else return 1;                                         // 1 = notEmpty
}

void enqueue_file(char*path, char*name, file_q *q){
    pthread_mutex_lock(&q->lock);
    
    pthread_t self = pthread_self();
    if(DEBUG2) printf("Direc_Thread: %ld enquing_FILE() %s.\n",self,name);

    while(q->isFull == 1) {
        if(DEBUG) printf("Direc_Thread: %ld. File-Queue is full.\n",self);
        pthread_cond_wait(&q->enqueue_ready,&q->lock);
    }

    file_node *node = malloc(sizeof(file_node));
    node->path = path;
    node->file = name;
    q->files[q->stop] = node;
    q->stop++;

    if(q->stop == SIZE) q->stop = 0;
    if(q->start == q->stop) q->isFull = 1;

    pthread_mutex_unlock(&q->lock);
    pthread_cond_broadcast(&q->dequeue_ready);

    return;
}

file_node* dequeue_file(file_q *q) {
    pthread_mutex_lock(&q->lock);

    pthread_t self = pthread_self();    
    file_node *result;
    while (directories->workers != 0 && q->workers != 0) {  // both directory workers and files workers have to be active
        while (!q->isFull && (q->start == q->stop)){
            if(DEBUG2) printf("File_Thread: %ld. Waiting for dequeue_ready() for file-q.\n",self);
            q->workers--;                                   // file thread waiting, decrement workers
            if(q->workers == 0) break;
            pthread_cond_wait(&q->dequeue_ready,&q->lock);
        }
        if(q->workers == 0) break;                          // check for the last thread that is coming out of cond_wait
        q->stop--;
        result = q->files[q->stop];
        break;
    }

    pthread_mutex_unlock(&q->lock);
    pthread_cond_broadcast(&q->enqueue_ready);
    return result;
}




void direc_traversal(char* path) {
    dir = opendir(path);
    de = readdir(dir);
    chdir(path);
    while (de != NULL) {
        int x = stat(de->d_name,&statbuff);
        if (x == -1) perror(de->d_name);
        if (de->d_name[0] == '.' || (strncmp(de->d_name,"wrap",4) == 0)) {
            if (DEBUG) printf("File ignored.\n");
        }
        if (S_ISDIR(statbuff.st_mode) && de->d_name[0] != '.') {
            char *file = de->d_name;
            char* newpath = string_maker(path,file);
            enqueue_direc(newpath,directories);
        }
        if (S_ISREG(statbuff.st_mode)) {
            char *file = malloc(sizeof(char) * 256);
            strcpy(file,de->d_name);
            if(DEBUG) printf("%s\n",de->d_name);
            enqueue_file(path,file,files); 
        }
        de = readdir(dir);
    }
    closedir(dir);
    return;
}


// THREAD WORKER FUNCTIONS
// ----------------------


void* direc_worker(void* args)
{  
    if(DEBUG2) printf("Direc_Thr: %ld starting.\n",pthread_self());  
    while (1) {
        pthread_mutex_lock(&directories->lock);
        directories->workers++;                                 // threads are starting work, increment
        pthread_mutex_unlock(&directories->lock);
        char* temp = dequeue_direc(directories);                // dequeue directory
        if(temp == NULL) break;                                 // if nothing comes back, then we 
        else {
            //if (DEBUG2) printf("Direc_Thread: %ld. Direc_traversal with DEQUEUD_DIREC() %s\n",pthread_self(),temp);
            direc_traversal(temp);                              // do a direc_traversal of dqd() directory
            pthread_mutex_lock(&directories->lock);
            directories->workers--;                             // thread finished with this directory, 
            pthread_mutex_unlock(&directories->lock);
        }
    }

    pthread_cond_broadcast(&directories->dequeue_ready);        // wake up threads before exiting
    if(DEBUG2) printf("Direc_Thread: %ld exiting.\n",pthread_self());
    pthread_exit(0);
}


void* file_worker(void* args)
{
    while(1){
        pthread_mutex_lock(&files->lock);
        files->workers++;
        pthread_mutex_unlock(&files->lock);
        file_node* temp = dequeue_file(files);
        if (temp == NULL) break;
        else {
            //if (DEBUG2) printf("File_Thread: %ld. Dequeud(): %s\n",pthread_self(), temp->file);
            char s[300] = "wrap.";
            if((strncmp(temp->file,"wrap",4) == 0)) {
                if(DEBUG) printf("Already wrapped file.\n");
                continue;
            }
            char* input = string_maker(temp->path,temp->file);
            char* output = string_maker(temp->path, strcat(s,temp->file));
            int fd = open(input, O_RDWR|O_APPEND,0600);
            int fd1 = open(output, O_RDWR | O_CREAT | O_TRUNC| O_APPEND, 0600);
            word_wrap(fd,fd1);
            free(output);
            free(input);
            free(temp);
            close(fd);
            close(fd1);

            pthread_mutex_lock(&files->lock);
            files->workers--;
            pthread_mutex_unlock(&files->lock);
        } 
    }

    pthread_cond_broadcast(&files->dequeue_ready);
    pthread_cond_broadcast(&files->enqueue_ready);
    if(DEBUG2) printf("File_THR:%ld exiting.\n",pthread_self());
    pthread_exit(0);
}





void print_files(file_q *q) {
    int ptr = q->start;
    if(q->stop == 0) {
        if(DEBUG) printf("Queue is empty.\n");
    }
    while (ptr != q->stop) {
        printf("%s\n",q->files[ptr]->file);
        ptr++;
    }
    return;
}

void delete_file_q(file_q *q) {
    int ptr = q->start;
    while (ptr != q->stop) {
        free(q->files[ptr]);
        ptr++;
    }
    return;
}




// M = number_of_direc_threads; N = number_of_file_threads


int main(int argc, char** argv)
{
    if (argc < 2) {
        if (DEBUG) printf("Please supply arguments for the program");
    }
    
    char* first_argument = argv[1];

    if (first_argument[1] == 'r') {
        file_width = atoi(argv[2]);
        char* target_direc = argv[3];

        files = init_file_q();          // initialize queues
        directories = init_direc_q();
        char* path = getcwd(NULL,200);
        char* newpath = string_maker(path,target_direc);    // make string for target_direc

        if (strlen(first_argument) < 3) {
            M = 1;
            N = 1;
        }
        else if (strlen(first_argument) < 4){
            M = 1;
            N = atoi(&first_argument[2]);
            if (DEBUG) printf("M = direc_threads %d\n",M);
            if (DEBUG) printf("N = file_threads %d\n",N);
        }
        else {
            char *comma;                        // find where the comma is in the string
            int index_comma;
            comma = strchr(first_argument,',');
            index_comma = (int) (comma - first_argument);
            printf("%d\n",index_comma);
            printf("%s\n",first_argument);
            M = (int) strtol(&first_argument[2],NULL,10);
            if (DEBUG) printf("M = direc_threads %d\n",M);
            first_argument = first_argument + index_comma+1;
            N = atoi(first_argument);
            if (DEBUG) printf("N = files_threads %d\n",N);
        }

        // create threads and queues for files and directories
        pthread_t direc_threads[M];
        pthread_t file_threads[N];

        enqueue_direc(newpath,directories);                                 // enq() first directory

        for (int i = 0; i < M; i++) {                                       // create and assign work to directory threads
            pthread_create(&direc_threads[i],NULL,direc_worker,NULL);
        }
        for (int i = 0; i < N; i++) {                                    // create and assign work to file threads
            pthread_create(&file_threads[i],NULL,file_worker,NULL);
        }
        for (int i = 0; i < M; i++) {                                       // join the directory threads after they are done
            pthread_join(direc_threads[i], NULL);
        }
        for (int i = 0; i < N; i++) {                                    // join the file threads after they are done
            pthread_join(file_threads[i], NULL);
        }


        //delete_direc_q(directories);
        delete_file_q(files);
        free(files);
        free(directories);
        free(path);
        free(newpath);
        return EXIT_SUCCESS;
    }

    if (argc  == 3) {
        if (check_input_digit(argv[1]) == -1){                                // check if the first argument is an integer. handles floats and negatives as well
            if (DEBUG) printf("Please supply an integer as the first argument. \n");
            return EXIT_FAILURE;
        }
        file_width = atoi(argv[1]);
        int x = stat(argv[2],&statbuff);
        if (x == -1) {
            perror(argv[2]);
            return EXIT_FAILURE;
        }
        if (S_ISREG(statbuff.st_mode)) {                                // given input is a regular file, works
            fd_in = open(argv[2],O_RDONLY);
            if (fd_in == -1) {
                perror(argv[2]);
                return EXIT_FAILURE;
            }
            word_wrap(fd_in,1);
            close(fd_in);
        }
        else if (S_ISDIR(statbuff.st_mode)) {                                                   // given input is a directory
            dir = opendir(argv[2]);
            while ((de = readdir(dir))) {
                if(de->d_name[0] == '.' || (strncmp(de->d_name,"wrap",4) == 0)) {
                    if (DEBUG) printf("File ignored.\n");
                    continue;                                                                   // ignore (.) and already wrapped files
                }
                char s[270] = "wrap.";                                                          // temporary string for output file
                chdir(argv[2]);
                fd_in = open(de->d_name, O_RDONLY);                                             // read current file in the directory
                fd_out = open(strcat(s,de->d_name),O_RDWR|O_CREAT|O_APPEND | O_TRUNC, 0600);    // create output file
                if (DEBUG) printf("Current file name is: %s\n", de->d_name);
                
                // if (DEBUG) printf("%s\n", getcwd(s, 100));
                word_wrap(fd_in,fd_out);
                close(fd_in);
                close(fd_out);
            }
        }
    }
  

    // works
    else if (argc == 2) {                                               // we will read from stdin and write to stdout
        if (check_input_digit(argv[1]) == -1){                                // check if the first argument is an integer
            if (DEBUG) printf("Please supply an integer as the first argument. \n");
            return EXIT_FAILURE;
        }
        file_width = atoi(argv[1]);
        word_wrap(0,1);                                                 // fd_in = stdin = 0, fd_out = stdout = 1
        close(0);
        close(1);
    }
    else {                                                              // need more arguments
        fd_in = 0;
        if (DEBUG) printf("Please supply file width and file name. \n");
        return EXIT_FAILURE;
    }

    if (success == -1) {                                                // if we were unsuccessful/if one word is longer than file_width
        if (DEBUG) printf("One word in the file was longer than the given file width! \n");
        return EXIT_FAILURE;
    }
    if (write_success == -1) {                                                // if we were unsuccessful/if one word is longer than file_width
        if (DEBUG) printf("Write() didn't execute properly. \n");
        return EXIT_FAILURE;
    }

    if (DEBUG) printf("\nProgram completed\n");


    return EXIT_SUCCESS;
}

/**
 * @brief What still needs to be added:
 * ------------------------------------
 * 
 * - Directory_traversal()
 * - Input configuration of "-r,3 4" etc. in main()
 * - File and directory queues/nodes
 * - Thread worker functions
 * 
 */




