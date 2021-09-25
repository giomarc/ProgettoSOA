#include "common_function.h"

extern int remove_tag(int tag);
extern int wakeup_tag(int tag);
extern int open_tag(int key);
extern int create_tag(int key, int permission);
extern int send_msg(int tag, int level, char *msg, unsigned int thread_id);
extern int receive_msg(int tag, int level, char *msg, unsigned int thread_id);
#define MAX_MSG_SIZE 4096


void signal_handler(int sig_num){
	fprintf(stdout, "Reader Process has been stopped , sig num = %d\n",sig_num);
	exit(EXIT_SUCCESS);
}

int main(int argc, char** argv){


	int td, ret;

	if(argc != 1){
	    printf("usage: <prog>\n");
	    return EXIT_FAILURE;
	}
        
	char buff[MAX_MSG_SIZE];

	signal(SIGTSTP,signal_handler);
	signal(SIGINT,signal_handler);

	td = create_tag(8, TAG_ACCESS_ALL);
	if(td == -1)
        return EXIT_FAILURE;

	sleep(2);
	ret = receive_msg(td, 1, buff, (unsigned int) pthread_self());
	if(ret == 0)
		printf("RECIVE this message: %s\n", buff);
	remove_tag(td);
	return 0;
}
	
