#include "common_function.h"

extern int remove_tag(int tag);
extern int wakeup_tag(int tag);
extern int open_tag(int key);
extern int create_tag(int key, int permission);
extern int send_msg(int tag, int level, char *msg, unsigned int thread_id);
extern int receive_msg(int tag, int level, char *msg, unsigned int thread_id);

//threads
#define NUM_READERS 1
#define NUM_WRITERS 1

pthread_t readers1[NUM_READERS];
pthread_t writers1[NUM_WRITERS];
pthread_t readers2[NUM_READERS];
pthread_t writers2[NUM_WRITERS];
pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;

#define NUM_OF_SERVICES 2
int tag_descriptors[NUM_OF_SERVICES];
int tag_keys[NUM_OF_SERVICES] = {10, 11};
int levels[NUM_OF_SERVICES] = {4,5};

char * mess[] = {" Hello!", "How are u?", "I'm Bond, James Bond", "May be the force with you"};

void sig_handler(int sig_num){
    fprintf(stdout, "Process has been stopped , sig num = %d\n",sig_num);
    exit(EXIT_SUCCESS);
}

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


//int tag_receive(int tag, int level, char* buffer, size_t size)
void* reader_work(void* args){

    struct thread_args * reader_arg = (struct thread_args *) args;
    char mess[MAX_MSG_SIZE];

    signal(SIGTSTP, sig_handler);
    signal(SIGINT, sig_handler);

    receive_msg(reader_arg->tag, reader_arg->level, mess, (unsigned int) pthread_self());
    return 0;

}



int main(int argc, char** argv){

    int i;

    if(argc != 1){
        fprintf(stdout, "No arguments required for %s\n", argv[0]);
        fflush(stdout);
        return EXIT_FAILURE;
    }

    signal(SIGTSTP, sig_handler);
    signal(SIGINT, sig_handler);

    for(i = 0; i < NUM_OF_SERVICES; i++)
        tag_descriptors[i] = create_tag(tag_keys[i], TAG_ACCESS_ALL);

    usleep(10);

    struct thread_args *arg1 = (struct thread_args *) malloc(sizeof(struct thread_args));
    struct thread_args *arg2 = (struct thread_args *) malloc(sizeof(struct thread_args));

    arg1->key = tag_keys[0];
    arg1->tag = tag_descriptors[0];
    arg1->level = levels[0];

    arg2->key = tag_keys[1];
    arg2->tag = tag_descriptors[1];
    arg2->level = levels[1];


    ///KEY 10, LEVEL 4: in this case, no wake up because of no reader pending
    if (NUM_READERS>0) {
        for (i = 0; i < NUM_READERS; i++) {
            if (pthread_create(&readers1[i], NULL, &reader_work, (void *) arg1)) {
                fprintf(stderr, "reader pthread_create() failed\n");
                fflush(stdout);
                return (EXIT_FAILURE);
            }
        }
    }
    sleep(1);
    if (NUM_WRITERS>0) {
        for (i = NUM_WRITERS; i < NUM_WRITERS*2; i++) {
            if (pthread_create(&writers1[i], NULL, &writer_work, (void *) arg1)) {
                fprintf(stderr, "reader pthread_create() failed\n");
                fflush(stdout);
                return (EXIT_FAILURE);
            }
        }
    }
    //readers found

    ///KEY 11, LEVEL 5: in this case, readers of level 5 must be waken up
    if (NUM_WRITERS>0) {
        for (i = 0; i < NUM_WRITERS; i++) {
            if (pthread_create(&writers2[i], NULL, &writer_work, (void *) arg2)) {
                fprintf(stderr, "reader pthread_create() failed\n");
                fflush(stdout);
                return (EXIT_FAILURE);
            }
        }
    }
    sleep(1);
    if (NUM_READERS>0) {
        for (i = NUM_READERS; i < NUM_READERS*2; i++) {
            if (pthread_create(&readers2[i], NULL, &reader_work, (void *) arg2)) {
                fprintf(stderr, "reader pthread_create() failed\n");
                fflush(stdout);
                return (EXIT_FAILURE);
            }
        }
    }

    sleep(1);
    wakeup_tag(arg1->tag);
    wakeup_tag(arg2->tag);
    remove_tag(arg1->tag);
    remove_tag(arg2->tag);
    printf("Exiting...\n");
    return EXIT_FAILURE;
}