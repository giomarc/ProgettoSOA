#include "common_function.h"

extern int remove_tag(int tag);
extern int wakeup_tag(int tag);
extern int open_tag(int key);
extern int create_tag(int key, int permission);
extern int send_msg(int tag, int level, char *msg, unsigned int thread_id);
extern int receive_msg(int tag, int level, char *msg, unsigned int thread_id);

//threads
#define NUM_READERS 2
#define NUM_WRITERS 2
#define KEY 12
char * mess[] = {"Hello!", "How are u?", "I'm Bond, James Bond", "May the force be with you", "I'm Harris, good job bro!"};
pthread_t readers[NUM_READERS][MAX_LEVELS];
pthread_t writers[NUM_WRITERS][MAX_LEVELS];

struct thread_args {
    int key;
    int tag;
    int level;
};

void * writer_work(void* args) {

    struct thread_args * writer_arg = (struct thread_args *) args;
    int which_msg = rand() % 5;

    send_msg(writer_arg->tag, writer_arg->level, mess[which_msg], (unsigned int) pthread_self());
    return 0;
}

void* reader_work(void* args){

    struct thread_args * reader_arg = (struct thread_args *) args;
    char mesg[MAX_MSG_SIZE];


    receive_msg(reader_arg->tag, reader_arg->level, mesg, (unsigned int) pthread_self());
    
    return 0;

}

int main(int argc, char** argv){

    int i,j,k,tag_descriptor,ret;
    struct thread_args *arg[MAX_LEVELS];

    if(argc != 1){
        fprintf(stdout, "No arguments required for %s\n", argv[0]);
        fflush(stdout);
        return EXIT_FAILURE;
    }


    tag_descriptor = create_tag(KEY, TAG_ACCESS_ALL);
    if(tag_descriptor == -2){        
        tag_descriptor = open_tag(KEY);
        if(tag_descriptor <0){
            return EXIT_FAILURE;
        }
    }

    //sleep(2);
    for(i = 0; i < MAX_LEVELS; i++){
        struct thread_args *arg1 = (struct thread_args *) malloc(sizeof(struct thread_args));
        arg1->key = KEY;
        arg1->tag = tag_descriptor;
        arg1->level = i;
        arg[i]= arg1;
    }

    for(j = 0; j < NUM_READERS; j++){
        for(k = 0;k < MAX_LEVELS; k++){
            if (pthread_create(&readers[j][k], NULL, &reader_work, (void *) arg[k])) {
                fprintf(stderr, "reader pthread_create() failed\n");
                fflush(stdout);
                return (EXIT_FAILURE);
            } 
        }
           
    }
    //sleep(5);
    for(j = 0; j < NUM_WRITERS; j++){
        for(k = 0; k < MAX_LEVELS; k++){
            if (pthread_create(&writers[j][k], NULL, &writer_work, (void *) arg[k])) {
                fprintf(stderr, "writer pthread_create() failed\n");
                fflush(stdout);
                return (EXIT_FAILURE);
            } 
        }           
    }

    sleep(1);
    ret = remove_tag(tag_descriptor);
    if(ret < 0)
        printf("Errore nella remove_tag %d\n",ret);
    printf("Exiting...\n");
    return EXIT_FAILURE;
}




