#include "common_function.h"

int remove_tag(int tag)
{
    int res = -1;
    if(tag < 0 || tag > MAX_TAG_SERVICES - 1){
        fprintf(stdout,"Tag descriptor must be between 0 and %d\n", MAX_TAG_SERVICES - 1);
        return -1;
    }

    res = syscall(TAG_CTL, tag, TAG_REMOVE);
    switch (res){
        case ER_SEARCH_TAG:
            fprintf(stdout,"Tag service %d : not found\n",tag);
            break;
        case ER_INTERNAL:
            fprintf(stdout,"Tag service %d : error in kmalloc\n",tag);
            break;
        case 0:
            fprintf(stdout, "Tag service %d : removed with success, returned %d \n", tag, res);
            return 0;
        case ER_RMV:
            fprintf(stdout, "Tag service %d : remove failed because of pending readers\n", tag);
            break;
       case ER_CMD:
            fprintf(stdout, "Tag service %d : command not valid for removal\n", tag);
            break;
        case ER_ACCESS:
            fprintf(stdout,"Tag service %d : permission denied\n",tag);
            break;  
        default:
            fprintf(stdout, "Tag service %d : something went wrong\n", tag);
            break;

    }
    return res ;
}

int send_msg(int tag, int level, char *msg, unsigned int thread_id)
{
    int ret;

    if(tag < 0 || tag > MAX_TAG_SERVICES - 1 || level > MAX_LEVELS - 1){
        fprintf(stdout,"Tag descriptor must be between 0 and %d and levels must be between 0 and %d\n",MAX_TAG_SERVICES - 1, MAX_LEVELS - 1);
        return -1;
    }

    ret = syscall(TAG_SEND, tag, level, msg, strlen(msg));
    switch (ret){
        case ER_SEARCH_TAG:
            fprintf(stdout,"SEND: tag service %d, not found\n",tag);
            break;
         case ER_ALLOC:
            fprintf(stdout,"SEND: tag service %d, error allocating message\n",tag);
            break;
        case ER_SEARCH_LEVEL:
            fprintf(stdout,"SEND: tag service %d, level %d not found\n", tag, level);
            break;
        case ER_ZALLOC:
            fprintf(stdout,"SEND: tag service %d, error in kzalloc\n",tag);
            break;
        case ER_NO_SLEEPERS:
            fprintf(stdout,"SEND: tag service %d level %d, there are no waiting sleepers\n", tag, level);
            break;
        case ER_ACCESS:
            fprintf(stdout,"SEND: tag service %d, permission denied\n",tag);
            break;  
        default:
            fprintf(stdout,"SEND: message %s, correctly delivered by %u to tag %d level %d\n", msg, thread_id, tag, level);
            return 0;
    }
  
    fflush(stdout);
    return -1;
}


int receive_msg(int tag, int level, char *msg, unsigned int thread_id)
{
    int ret;

    ret = syscall(TAG_RECEIVE, tag, level, msg, MAX_MSG_SIZE);
    switch(ret){
        case ER_SEARCH_TAG:
            fprintf(stdout,"RECEIVE: tag service %d, not found\n", tag);
            break;
        case ER_INTERNAL:
            fprintf(stdout,"RECEIVE: tag service %d, error in kmalloc\n", tag);
            break;
        case ER_SEARCH_LEVEL:
            fprintf(stdout,"RECEIVE: tag service %d, level %d not found\n",tag, level);
            break;
        case ER_BADSIZE:
            fprintf(stdout,"RECEIVE: tag service %d, error in message size\n",tag);
            break;
        case ER_LOCK:
            fprintf(stdout,"RECEIVE: read lock busy error\n" );
            break;
        case ER_ALLOC:
            fprintf(stdout,"RECEIVE: error in kzalloc\n");
            break;
        case 0:
            fprintf(stdout,"RECEIVE: Thread %u, tag %d, level %d, read correctly message: %s\n", thread_id, tag, level, msg);
            return 0;
        case ER_MYSIG:
            fprintf(stdout,"RECEIVE: Thread %u woke up because of a signal\n",thread_id);
            break;
        case ER_AWAKE:
            fprintf(stdout,"RECEIVE: Thread %u error in waiting queue\n",thread_id);
            break;
        case ER_ACCESS:
            fprintf(stdout,"RECEIVE: tag service %d, permission denied\n",tag);
            break;  
        default:
            fprintf(stdout,"RECEIVE: error while reading\n");
            break;
    }
    fflush(stdout);
    return -1 ;
}


int wakeup_tag(int tag)
{
    int res;

    res = syscall(TAG_CTL, tag, TAG_WAKEUP);
    switch(res){
        case ER_CMD:
            fprintf(stdout, "WAKE UP: tag service %d, command not valid for wake up\n", tag);
            break;
        case ER_SEARCH_TAG:
            fprintf(stdout,"WAKE UP: tag service %d not found\n", tag);
            break;
        case ER_ALLOC:
            fprintf(stdout,"WAKE UP: tag service %d error allocating message\n", tag);
            break;
        case ER_ACCESS:
            fprintf(stdout,"WAKE UP: tag service %d, permission denied\n",tag);
            break;
        case ER_AWAKE:
            fprintf(stdout,"WAKE UP: tag service %d, no readers to wake up!\n", tag);
            break;
        case 0:
            fprintf(stdout,"WAKE UP: tag_service %d, woke up all readers!\n", tag);
            return 0;
        default:
            fprintf(stdout,"WAKE UP: tag service %d, error waking up readers\n", tag);
            break;
    }

    fflush(stdout);
    return -1;
}



int open_tag(int key)
{
    int td;
	
    td = syscall(TAG_GET, key, TAG_OPEN);
	
    switch (td){
        case ER_OPEN:
            fprintf(stdout, "OPEN: cannot open again IPC_PRIVATE_KEY\n");
            return -1;
         case ER_ACCESS:
            fprintf(stdout, "OPEN: can't open tag service because has private access permission\n");
            return -1;
        case ER_INTERNAL:
            fprintf(stdout, "OPEN: no tag service found for this key %d\n", key);
            return -1;
        case ER_CMD:
            fprintf(stdout, "OPEN: invalid command\n");
            return -1;
        default:
            fprintf(stdout, "OPEN: tag_service %d with key %d opened successfully\n", td, key);
            return td;
    }
}
   


int create_tag(int key, int permission)
{
    
    int td;
    char *perm = "TAG_ACCESS_ALL";
    char *priv = "";

    if(permission == 1)
        perm = "TAG_ACCESS_PRIVATE";
    if(key == IPC_PRIVATE_KEY)
        priv = "IPC_PRIVATE_KEY";

    td = syscall(TAG_GET, key, TAG_CREATE, permission);

    switch (td){
		case ER_INTERNAL:
			fprintf(stdout,"CREATE: error in kmalloc for key %d\n",key);
            return -1;
        case ER_ACCESS:
            fprintf(stdout, "CREATE: access denied for key %d\n", key);
            return -1;
		case ER_CMD:
			fprintf(stdout,"CREATE: invalid command at key %d \n",key);
			return -1;	
		case ER_KEY:
            //user decides if open tag or not
			fprintf(stdout,"CREATE: key %d yet exists \n", key);
            /*if(key != IPC_PRIVATE_KEY){
                td_found = open_tag(td);
                return td_found;
            }else{
                printf("Cannot open IPC_PRIVATE\n");
                return -1;*/
            return open_tag(key);
            //}
		case ER_TAG_BUSY:
			fprintf(stdout,"CREATE: no more tag descriptors available for key %d\n",key);
			return -1;
        case ER_CREATE_TAG:
            fprintf(stdout,"CREATE: error creating tag service with key %d\n",key);
            return -1;  		
		default:
			fprintf(stdout,"CREATE: successful creation of key %d %s, tag descriptor %d, permission %s\n", key, priv, td, perm);
			break;

	}
    fflush(stdout);
    return td;
}
