#include "common_function.h"
#include <sys/ioctl.h>
#include <fcntl.h>

extern int remove_tag(int tag);
extern int wakeup_tag(int tag);
extern int open_tag(int key);
extern int create_tag(int key, int permission);
extern int send_msg(int tag, int level, char *msg, unsigned int thread_id);
extern int receive_msg(int tag, int level, char *msg, unsigned int thread_id);

#define MAX_SIZE 8192
#define LINE_LENGTH 60
#define TAG_TO_CREATE 256
int i;
char buff[MAX_SIZE];


void * the_thread(void* path){
    char* device;
    int fd, ret;
    char mess[MAX_SIZE*LINE_LENGTH];

    device = (char*) path;

    fd = open(device, O_RDWR);
    if(fd < 0) {
        printf("Error opening device %s\n",device);
        pthread_exit(EXIT_SUCCESS);
    }
    fprintf(stdout, "device %s successfully opened\n",device);
    size_t len = MAX_SIZE*LINE_LENGTH;
    fprintf(stdout, "********************** threads %ld ******************************\n", pthread_self());

    ret = read(fd,mess,len);
    if (ret < 0)
        fprintf(stdout, "No state to show %s\n", strerror(errno));
    else
        fprintf(stdout, "Current tag service state is \n%s\n", mess);

    close(fd);
    pthread_exit(EXIT_SUCCESS);

}

int main(int argc, char** argv){
    int major, minors, ret, i, created_tag = 0;
    char *path;
    int tag_descriptor[TAG_TO_CREATE];

    fflush(stdout);
    if(argc < 4){
        printf("Usage: <prog> <device_path> <major> <minors>\n");
        return -1;
    }

    path = argv[1];
    major = strtol(argv[2],NULL,10);
    minors = strtol(argv[3],NULL,10);
    pthread_t tid[minors];

    fprintf(stdout, "Creating %d minors for device %s with major %d\n", minors, path, major);

    fprintf(stdout, "Starting creating tag services... \n");
    for( i = 0; i < TAG_TO_CREATE; i++){
        tag_descriptor[i] = create_tag(i, TAG_ACCESS_ALL);
        if(tag_descriptor[i] == -1) break;
        created_tag++;
    }
    printf("Created tag %d\n", created_tag);
    
    sleep(1);
    fprintf(stdout, "Starting creating threads... \n");
    for(i = 0; i < minors; i++){
        sprintf(buff,"mknod %s%d c %d %i\n", path, i, major, i);
        system(buff);
        sprintf(buff,"%s%d",path, i);
        printf("buff %s\n", buff);
        pthread_create(&tid[i], NULL, the_thread, strdup(buff));
    }


    fprintf(stdout, "Starting removing tag services... \n");
    //sleep(1);
    for(i = 0; i < minors; i++){
        pthread_join(tid[i], NULL);
    }
    for(i = 0; i < created_tag; i++){
        ret = remove_tag(tag_descriptor[i]);
        if(ret < 0)
            printf("ERROR removing tag = %d\n", i);
    }
    
    sprintf(buff,"rm  %s*\n",path);
    system(buff);
    printf("Exiting...\n");
    return 0;
}

	

	
