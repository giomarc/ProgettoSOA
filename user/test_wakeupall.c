#include "common_function.h"

extern int remove_tag(int tag);
extern int wakeup_tag(int tag);
extern int open_tag(int key);
extern int create_tag(int key, int permission);
extern int send_msg(int tag, int level, char *msg, unsigned int thread_id);
extern int receive_msg(int tag, int level, char *msg, unsigned int thread_id);


#define NUM_READERS 2
#define MAX_LEVEL 32 // QUESTA NON SERVE PERCHÈ HO IN CONFIGURE MAX_LEVELS
pthread_t readers[NUM_READERS][MAX_LEVEL];
#define KEY 12

struct thread_args {
    int key;
    int tag;
    int level;
};


void* reader_work(void* args){

    struct thread_args * reader_arg = (struct thread_args *) args;
    char mesg[MAX_MSG_SIZE];
    receive_msg(reader_arg->tag, reader_arg->level, mesg, (unsigned int) pthread_self());
    return 0;

}

int main(int argc, char** argv){

    int i, j, k, tag_descriptor, ret;
    struct thread_args *arg[MAX_LEVEL];

    if(argc != 1){
        fprintf(stdout, "Usage: %s\n", argv[0]);
        fflush(stdout);
        return EXIT_FAILURE;
    }

    tag_descriptor = create_tag(KEY, TAG_ACCESS_ALL);
    if(tag_descriptor == -1) return EXIT_FAILURE;


    usleep(10);
    for(i = 0; i < MAX_LEVEL; i++){
        struct thread_args *arg1 = (struct thread_args *) malloc(sizeof(struct thread_args));
        arg1->key = KEY;
        arg1->tag = tag_descriptor;
        arg1->level = i;
        arg[i]= arg1;
    }

    for(j = 0; j < NUM_READERS; j++){
        for(k = 0; k < MAX_LEVEL; k++){
            if (pthread_create(&readers[j][k], NULL, &reader_work, (void *) arg[k])) {
                fprintf(stderr, "reader pthread_create() failed\n");
            
                return (EXIT_FAILURE);
            } 
        }           
    }

    sleep(2);
    wakeup_tag(tag_descriptor);
    
    for(j = 0; j < NUM_READERS; j++){
        for(k = 0; k < MAX_LEVEL; k++){
             pthread_join(readers[j][k],NULL);
        }
    }
    ret = remove_tag(tag_descriptor);
    if(ret < 0) return EXIT_FAILURE;
    printf("Exiting...\n");
 
    return EXIT_SUCCESS;
}




