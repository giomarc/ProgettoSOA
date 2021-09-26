#include "common_function.h"

extern int remove_tag(int tag);
extern int wakeup_tag(int tag);
extern int open_tag(int key);
extern int create_tag(int key, int permission);
extern int send_msg(int tag, int level, char *msg, unsigned int thread_id);
extern int receive_msg(int tag, int level, char *msg, unsigned int thread_id);

#define MAX_MSG_SIZE 4096
int td;

void signal_handler(int sig_num){
	fprintf(stdout, "Reader Process has been stopped , sig num = %d\n",sig_num);
	remove_tag(td);
	exit(EXIT_SUCCESS);
}

int main(int argc, char** argv){
	
	int ret, permission;
	bool exit = true;
	
	if(argc < 4){
	    fprintf(stdout, "usage: <prog> <key> <permission> <level>\n");
	    return EXIT_FAILURE;
	}
        
	char buff[MAX_MSG_SIZE];
	int key = strtol(argv[1], NULL, 10);
	if(strcmp(argv[2], "TAG_ACCESS_ALL") == 0) permission = TAG_ACCESS_ALL;
	else if (strcmp(argv[2], "TAG_ACCESS_PRIVATE")== 0) permission = TAG_ACCESS_PRIVATE;
	else{
	    fprintf(stdout, "Permission must be or TAG_ACCESS_ALL or TAG_ACCESS_PRIVATE\n");
        return EXIT_FAILURE;
	}
	int level = strtol(argv[3], NULL, 10);
    if(level < 0 || level > 31)
    {
        fprintf(stdout, "Level must be between 0 and 31\n");
        return EXIT_FAILURE;
    }
	
	signal(SIGTSTP,signal_handler);
	signal(SIGINT,signal_handler);

	td = create_tag(key, permission);
	if(td < 0) return EXIT_FAILURE;

	sleep(1);
	fprintf(stdout, "Waiting for new messages\n");
    while (exit) {
        ret = receive_msg(td, level, buff, (unsigned int) pthread_self());
        if(ret != 0) break;
        sleep(1);
    }

	remove_tag(td);
	return 0;
}
	
