#include "common_function.h"

extern int remove_tag(int tag);
extern int wakeup_tag(int tag);
extern int open_tag(int key);
extern int create_tag(int key, int permission);
extern int send_msg(int tag, int level, char *msg, unsigned int thread_id);
extern int receive_msg(int tag, int level, char *msg, unsigned int thread_id);

#define NUM_TAG_SERVICE 256

int main(int argc, char** argv){

    int i, created_tag, ipc_tag, full_td, ret, res;
    int tag_descriptor[NUM_TAG_SERVICE];

    if(argc != 1){
        fprintf(stdout, "No arguments required for %s\n", argv[0]);
        fflush(stdout);
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Creating tags...\n");
    for(i = 0; i < NUM_TAG_SERVICE; i++){
        tag_descriptor[i] = create_tag(i, TAG_ACCESS_ALL);
        if(tag_descriptor[i] == -1) break;
    }

    full_td = create_tag(257, TAG_ACCESS_ALL); //testing tag descriptor queue full
    res = remove_tag(tag_descriptor[5]); //testing removing tag
    ipc_tag = create_tag(IPC_PRIVATE_KEY, TAG_ACCESS_PRIVATE); //trying to create IPC_PRIVATE

    sleep(1);
    fprintf(stdout, "Removing tags...\n");
    created_tag = sizeof(tag_descriptor)/sizeof(tag_descriptor[0]);
    for(i = 0; i < created_tag; i++){
        ret = remove_tag(tag_descriptor[i]);
        if(ret < 0)
            fprintf(stdout, "Error removing tag %d\n", tag_descriptor[i]);
    }

    fprintf(stdout, "Exiting...\n");
    return EXIT_SUCCESS;
}




