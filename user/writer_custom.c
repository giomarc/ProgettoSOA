#include "common_function.h"

extern int remove_tag(int tag);
extern int wakeup_tag(int tag);
extern int open_tag(int key);
extern int create_tag(int key, int permission);
extern int send_msg(int tag, int level, char *msg, unsigned int thread_id);
extern int receive_msg(int tag, int level, char *msg, unsigned int thread_id);

#define MAX_MSG_SIZE 4096
#ifdef FAULT
char *msg = NULL;
#else
char msg[MAX_MSG_SIZE];
#endif


void signal_handler(int sig_num){
    fprintf(stdout, "Writer Process has been stopped , sig num = %d\n",sig_num);
    exit(EXIT_SUCCESS);
}

int main(int argc, char** argv){

    int td, level, key;

    if(argc < 3){
        printf("usage: <prog> <key> <level>\n");
        return 0;
    }

    key = strtol(argv[1], NULL, 10);
    level = strtol(argv[2], NULL, 10);
    if(level < 0 || level > 31)
    {
        fprintf(stdout, "Level must be between 0 and 31\n");
        return EXIT_FAILURE;
    }

    signal(SIGTSTP,signal_handler);
    signal(SIGINT,signal_handler);

    td = open_tag(key);
    if(td < 0) return EXIT_FAILURE;

    while (!feof(stdin)) {

        fprintf(stdout, "\nWrite a new message or digit QUIT to exit\n");

        if(fgets(msg, MAX_MSG_SIZE, stdin) < 0){
            fprintf(stderr,"Error in fgets: %s\n",strerror(errno));
            return EXIT_FAILURE;
        }

        if(strcmp(msg, "QUIT\n") != 0)
            send_msg(td, level, msg, pthread_self());
        else{
            break;
        }

    }
    return  EXIT_SUCCESS;
}