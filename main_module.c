#include "main_module.h"

extern void enqueue(struct available_tag_descriptor* queue, int tag_descriptor);
extern int dequeue(struct available_tag_descriptor* queue);
extern int first(struct available_tag_descriptor* queue);

//unsigned long sys_call_table_address = 0x0;
module_param(sys_call_table_address, ulong, 0660);
//unsigned long sys_ni_syscall_address = 0x0;
module_param(sys_ni_syscall_address, ulong, 0660);
int free_entries[MAX_FREE];
module_param_array(free_entries,int,NULL,0660);//default array size already known - here we expose what entries are free



//--------------------------------------------------- DRIVER -----------------------------------------------------------

//DRIVER
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
#define get_major(session)	MAJOR(session->f_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_inode->i_rdev)
#else
#define get_major(session)	MAJOR(session->f_dentry->d_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_dentry->d_inode->i_rdev)
#endif
int Major;
object_state objects[MINORS];
spinlock_t driver_spinlock;

int dev_open(struct inode *inode, struct file *file) {

    int minor = get_minor(file);
    printk("%s: DRIVER minor %d \n", MODNAME, minor);

    if(minor >= MINORS){
    	printk("%s: DRIVER error minor>=MINORS\n", MODNAME);
        return -ENODEV;
    }

    if (!mutex_trylock(&(objects[minor].object_busy))) {
    	printk("%s: DRIVER single session error\n",MODNAME);
		goto open_failure;
    }

    printk("%s: DRIVER device file successfully opened for object with minor %d\n",MODNAME,minor);
    return 0;

open_failure:
    return -EBUSY;
}

int dev_release(struct inode *inode, struct file *file) {

    int minor = get_minor(file);

    mutex_unlock(&(objects[minor].object_busy));

    printk("%s: device file closed\n",MODNAME);
    return 0;
}


//crea lo snapshoot del sistema da restituire
void status_builder(char * service_status)
{
    int j, len = 0;
    ssize_t old_size = 0;
    unsigned bkt;
    struct tag_service *current_tag;
    struct level *tag_level;
    char *temp = (char *) vmalloc(sizeof(char) * MAX_LINE);
    if(temp ==NULL){
        printk("%s: DRIVER error allocating status builder\n",MODNAME);
   }

   printk("%s, SONO IN STATUS BUILDER\n", MODNAME);
   spin_lock(&driver_spinlock);
   strcpy(service_status, "");
   if(service_status == NULL){
   	   printk("%s: DRIVER error allocating row\n",MODNAME);
   }

   hash_for_each_rcu(tbl, bkt, current_tag, node){
	    if (current_tag != NULL){
            for (j = 0; j < MAX_LEVELS; j++) {
                tag_level = &(current_tag->levels[j]);
                strcpy(temp, "");
                sprintf(temp,"TAG-key %d TAG-creator %d TAG-level %d Waiting-threads %d\n", current_tag->key, current_tag->owner, j, tag_level->sleepers);
                memcpy(service_status+old_size,temp,strlen(temp));
				old_size += strlen(temp);
               }
     	}
   }

   len = strlen(service_status);
   if (len == 0) {
      printk("%s: No active service\n", MODNAME);
      spin_unlock(&driver_spinlock);
      return;
   }
   service_status[old_size] = '\0';
   spin_unlock(&driver_spinlock);
}


ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off)
{
    int ret, valid_bytes;
    int minor = get_minor(filp);
    object_state *the_object;
    the_object = objects+minor;

    printk("%s: DRIVER called read by minor: %d \n", MODNAME,minor);

    status_builder(the_object->stream_content);
    valid_bytes = strlen(the_object->stream_content);

    if (*off > valid_bytes) return 0;
    if ((valid_bytes - *off) < len) len = valid_bytes - *off;

    ret = copy_to_user(buff,&(the_object->stream_content[*off]),len);
    *off += (len - ret);

    return len - ret;
}


struct file_operations fops = {
        .owner = THIS_MODULE,
        .read = dev_read,
        .open =  dev_open,
        .release = dev_release
};


int init_driver(void) {

    int i;

    spin_lock_init(&driver_spinlock);

    for(i = 0; i < MINORS; i++){
        mutex_init(&(objects[i].object_busy));
        mutex_init(&(objects[i].operation_synchronizer));
        objects[i].valid_bytes = 0;
        objects[i].stream_content = NULL;
        objects[i].stream_content =(char*)vmalloc(sizeof(char)*MAX_SIZE*MAX_LINE);

        if(objects[i].stream_content == NULL) goto revert_allocation;
    }

    Major = __register_chrdev(0, 0, 256, DEVICE_NAME, &fops);

    if (Major < 0) {
        printk("%s: !!! DRIVER register failed !!!\n",MODNAME);
        return Major;
    }

    printk("%s: *** DRIVER new device registered, Major = %d ***\n",MODNAME, Major);
    return 0;

    revert_allocation:
    for( ; i >= 0; i--){
        vfree(objects[i].stream_content);
    }
    return -ENOMEM;
}

void cleanup_driver(void) {

    int i;
    for(i = 0; i < MINORS; i++){        
        vfree(objects[i].stream_content);        
    }

    unregister_chrdev(Major, DEVICE_NAME);
    printk("%s: *** DRIVER device unregistered with Major = %d ***\n",MODNAME, Major);
    return;

}

#define AUDIT if(1)


// --------------------------------------------------- MAIN MODULE -------------------------------------------------------

//Tag_get
#ifdef SYS_CALL_INSTALL
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(3, _tag_get, int, key, int, command, int, permission){
#else
asmlinkage int  sys_tag_get(int key, int command, int permission){
#endif

	struct tag_service *tag = NULL;
	int key_tag, res;

	switch (command) {
        case TAG_OPEN:

            if(key < 0){
                printk("%s: CREATE not allowed to create this key %d\n", MODNAME, key);
                return ER_ACCESS;
            }

            key_tag = myhash(key);
            tag = search_tag_by_key(key, key_tag, TAG_OPEN);

            //check
            if(tag == NULL)
                return ER_INTERNAL; //no tag corresponding to this key
            if(key == IPC_PRIVATE_KEY){
                printk("%s: key is IPC_PRIVATE, cannot open this tag service \n",MODNAME);
                return ER_OPEN; //cannot open IPC_PRIVATE KEY
            }
            if(check_access_perm(tag) != 0)
                return ER_ACCESS;
            break;

		case TAG_CREATE:

			if(key < 0){
				printk("%s: CREATE not allowed to create this key %d\n", MODNAME, key);
				return ER_ACCESS;
			}
			if(is_full(tag_queue)){
                printk("%s: CREATE all tag service are in usage\n", MODNAME);
                return ER_TAG_BUSY; //no more tag descriptors available
			}

			//Checking if there is yet a tag descriptor for that key
            key_tag = myhash(key);
            tag = search_tag_by_key(key, key_tag, TAG_CREATE);

            if(tag != NULL){
                printk("%s: CREATE error, key yet exists %d\n", MODNAME,key);
                return ER_KEY;
            }
            //creating tag service
			tag = kmalloc(sizeof(struct tag_service), GFP_KERNEL);
			if (tag == NULL){
				printk("%s: CREATE error while allocating tag service with key %d\n", MODNAME,key);
				return ER_INTERNAL;
			}
			res = create_tag_service(tag, key, permission);
			if(res < 0){
				printk("%s: CREATE error creating tag service with key %d\n", MODNAME,key);
				return ER_CREATE_TAG;
			}
			break;

		default:
			printk("%s: Please, insert TAG_OPEN or TAG_CREATE as command\n", MODNAME);
			return ER_CMD;
	}
	return tag->tag_descriptor;
}



//Tag_send
#ifdef SYS_CALL_INSTALL
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(4, _tag_send, int, tag, int, level, char*, buffer, size_t, size){
#else
asmlinkage int sys_tag_send(int tag, int level, char* buffer, size_t size){
#endif
	struct tag_service *tag_service;
	struct level * tag_level;
	struct message *msg;
	struct thread_data *cursor, *temp;
	char * addr;
	int ret = 0, how_many_readers, flag, prev_val;

	tag_service = search_tag_by_tagdescriptor(tag);
	if(tag_service == NULL) return ER_SEARCH_TAG; //if tag service does not exists, notify error

	if(check_access_perm(tag_service) != 0 ){
	    //if thread hasn't same id as owner, notify error
		printk("%s: SEND NOT allowed becaus of PRIVATE ACCESS: thread is %d and owner is %d\n",MODNAME, (int)current->tgid, tag_service->owner);
		return ER_ACCESS;
	}

	rcu_read_lock();
	tag_level = &(tag_service->levels[level]);
	if(tag_level == NULL){
		rcu_read_unlock();
		return ER_SEARCH_LEVEL;
	}
	rcu_read_unlock();
	
	addr = kzalloc(MAX_MSG_SIZE, GFP_KERNEL);
    if(addr == NULL){
    	printk("%s: SEND error kmalloc addr for tag_service = %d\n",MODNAME, tag);
    	return ER_ZALLOC;	
    }

    //copying message from user
    ret = copy_from_user((char*)addr, (char*)buffer, size);
	how_many_readers = __atomic_load_n(&tag_level->sleepers, __ATOMIC_SEQ_CST);
	flag = __atomic_load_n(&tag_level->flag, __ATOMIC_SEQ_CST);

	//if there are no waiting readers, delete message and return
	if(how_many_readers == 0){        
        printk("%s: SEND no readers found for tag_service %d, how_many_readers %d\n", MODNAME, tag, how_many_readers);
        kfree(addr);
        addr = NULL;
        return ER_NO_SLEEPERS;
    }

	//if there are waiting readers, send message
	else if(flag == 0 && how_many_readers > 0)
    {
	    msg = tag_level->msg;
        msg->readers = how_many_readers;

        msg->text = kzalloc(MAX_MSG_SIZE,GFP_KERNEL);
        if(msg->text ==NULL){
        	printk("%s: SEND ERROR KMALLOC Message\n", MODNAME);  
        	return ER_ALLOC;
        }

        write_lock(&tag_level->lock);
        memcpy((char *)msg->text, (char*) addr, strlen(addr));
        kfree(addr);
		addr = NULL;
        write_unlock(&tag_level->lock);

        msg->msg_len = (int) strlen(msg->text);
        __atomic_exchange(&tag_level->flag, &how_many_readers, &prev_val,__ATOMIC_SEQ_CST);

        //wake up sleeping threads
        list_for_each_entry_safe(cursor, temp, &(tag_level->reader_sleepers), list){
            wake_up_process(cursor->t);
        }
        remove_thread_data(tag_level);
    }
	return ret;
}



//Tag_receive
#ifdef SYS_CALL_INSTALL
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(4, _tag_receive, int, tag, int, level, char*, buffer, size_t, size){
#else
asmlinkage int sys_tag_receive(int tag, int level, char* buffer, size_t size){
#endif
	struct tag_service *tag_service;
	struct level * tag_level;
	struct message *msg_addr;
	struct thread_data *td;
	char * addr;
	int not_busy,wait_read,ret = 0,how_many_readers,flag;
	size_t len;

    tag_service = search_tag_by_tagdescriptor(tag);
	if(tag_service == NULL){
		return ER_SEARCH_TAG;
	}
	
	if(check_access_perm(tag_service) != 0){
		printk("%s: RECEIVE NOT allowed because of PRIVATE ACCESS\n",MODNAME);
		return ER_ACCESS;
	}

	//if no one is removing tag service
	not_busy = read_trylock(&tag_service->lock);
	if(!not_busy){
		printk("%s: RECEIVE lock busy\n", MODNAME);
		read_unlock(&tag_service->lock);
		return ER_LOCK;
	}

    tag_level = &(tag_service->levels[level]);
	if(tag_level == NULL) {
	    read_unlock(&tag_service->lock);
	    return ER_SEARCH_LEVEL;
	}
	read_unlock(&tag_service->lock);

	//Allocating thread data struct for reader informations
	td = kmalloc(sizeof(struct thread_data), GFP_KERNEL);
	if (td == NULL){
		printk("%s: RECEIVE error kmalloc thread_data\n", MODNAME);	
		return ER_INTERNAL;
	}

	//If I'm the first reader, allocate struct msg to let writer to write in
	if(tag_level->msg == NULL){
		msg_addr = kmalloc(sizeof(struct message), GFP_KERNEL);
		if (msg_addr == NULL){
			printk("%s: RECEIVE error kmalloc msg_addr\n", MODNAME);	
			return ER_INTERNAL;
		}
        tag_level->msg = msg_addr;
	}

	//Initialization of thread struct
	td->msg_ptr_addr = &tag_level->msg;
	td->t = current;
	//Put thread (reader) in sleepers list
	__atomic_fetch_add(&tag_level->sleepers, 1 , __ATOMIC_SEQ_CST);
	list_add_tail_rcu(&td->list, &tag_level->reader_sleepers);

	addr = kzalloc(MAX_MSG_SIZE, GFP_KERNEL); //dealloco dopo la copy to user
    if (addr == NULL){
    	printk("%s: RECEIVE error KMALLOC addr\n",MODNAME);
    	return ER_ALLOC;
    } 
    //thread in wait_queue until condition is not verified
    wait_read = wait_event_interruptible(tag_level->wait_queue, tag_level->flag>0 && tag_level->flag == tag_level->sleepers);

    switch(wait_read){
		case 0:
			printk("%s: RECEIVE LIVELLO %d,sono il thread current %p \n", MODNAME,tag_level->id,current);

    		len = (size_t) (tag_level->msg)->msg_len;
    		if(len > size) return ER_BADSIZE;

    		//get message from sender
            read_lock(&tag_level->lock);
    		memcpy(addr,(tag_level->msg)->text,len);
			read_unlock(&tag_level->lock);

			//copy message to user
    		ret = copy_to_user((char*)buffer,(char*)addr,size);
    		kfree(addr);
    		addr=NULL;

			how_many_readers = __atomic_load_n(&tag_level->sleepers, __ATOMIC_SEQ_CST);
			flag = __atomic_load_n(&tag_level->flag, __ATOMIC_SEQ_CST);
    		__atomic_fetch_sub(&tag_level->sleepers,1,__ATOMIC_SEQ_CST);
    		__atomic_fetch_sub(&tag_level->flag,1,__ATOMIC_SEQ_CST);

    		//if i'm the last reader
    		how_many_readers = __atomic_load_n(&tag_level->sleepers, __ATOMIC_SEQ_CST);
    		if (how_many_readers == 0){
    			if(((tag_level->msg)->text) != NULL){
    				kfree(tag_level->msg->text);
                    tag_level->msg->text = NULL;
    			}
    			kfree(tag_level->msg);
                tag_level->msg = NULL;
    		}
    		break;
    	
    	case -ERESTARTSYS:
    		printk("%s: RECEIVE signal wake up %p\n",MODNAME,current);
    		td->msg_ptr_addr = NULL;
    		kfree(addr);
    		addr = NULL;
    		__atomic_fetch_sub(&tag_level->sleepers, 1, __ATOMIC_SEQ_CST);
    		ret = ER_MYSIG;
    		break;
    	
    	default:
    		printk("%s: RECEIVE error in readers waiting queue\n", MODNAME);
    		kfree(addr);
    		addr = NULL;    		
    		__atomic_fetch_sub(&tag_level->sleepers, 1, __ATOMIC_SEQ_CST);
    		ret = ER_AWAKE;
    		break;
    }	
   		
	return ret;
}


#ifdef SYS_CALL_INSTALL
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(2, _tag_ctl, int, tag, int, command){
#else
asmlinkage int sys_tag_ctl(int tag, int command){
#endif
	struct tag_service *tag_service;
	int ret;

	switch (command){
		case TAG_REMOVE:

            tag_service = search_tag_by_tagdescriptor(tag);
            if(tag_service == NULL){
                return ER_SEARCH_TAG;
            }

			write_lock(&tag_service->lock);

			//if there are sleeping readers, cannot remove tag service
			if(check_readers(tag_service) < 0){ //yet pending readers
				write_unlock(&tag_service->lock);
				printk("%s: Cannot REMOVE tag %d because of pending readers \n", MODNAME, tag);
				return ER_RMV;
			}

            //else, remove
			hlist_del_rcu(&(tag_service->node));
            enqueue(tag_queue, tag);
			write_unlock(&tag_service->lock);
			kfree(tag_service);
			tag_service = NULL;
            printk("%s: REMOVED tag %d \n", MODNAME, tag);
			return 0;

		case TAG_WAKEUP:
		    //wake up sleeping readers
            tag_service = search_tag_by_tagdescriptor(tag);
            if(tag_service == NULL){
                return ER_SEARCH_TAG;
            }

			ret = wake_up_all_level(tag_service);
            if(ret == ER_ALLOC)
                return ER_ALLOC;
			if(ret == 0){
                printk("%s: AWAKE ALL there are no readers for this tag_descriptor %d\n", MODNAME, tag_service->tag_descriptor);
				return ER_AWAKE; 
			}else{
			    printk("%s: AWAKE ALL woke up all levels of tag_descriptor %d\n", MODNAME, tag_service->tag_descriptor);
                return 0;
			}

		default:
		    //wrong command, neither AWAKE_ALL_TAG or REMOVE_TAG
			return ER_CMD;
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
static unsigned long sys_tag_get = (unsigned long) __x64_sys_tag_get;
static unsigned long sys_tag_send = (unsigned long) __x64_sys_tag_send;	
static unsigned long sys_tag_receive = (unsigned long) __x64_sys_tag_receive;	
static unsigned long sys_tag_ctl = (unsigned long) __x64_sys_tag_ctl;	
#else
#endif
#endif
#endif
#endif

unsigned long cr0;

static inline void
write_cr0_forced(unsigned long val)
{
    unsigned long __force_order;

    asm volatile(
        "mov %0, %%cr0"
        : "+r"(val), "+m"(__force_order));
}

static inline void
protect_memory(void)
{
    write_cr0_forced(cr0);
}

static inline void
unprotect_memory(void)
{
    write_cr0_forced(cr0 & ~X86_CR0_WP);
}

#else
#endif


int init_module(void)
{
	//init hash table for tag servicesr
	hash_init(tbl);
	//init tag descriptor queue
	tag_queue = create_queue();
	printk("Available tag descriptors %d\n", tag_queue->available_tags);
  
	printk("%s: ******** initializing module ********n",MODNAME);
	syscall_table_finder();

	if(!hacked_syscall_tbl){
		printk("%s: failed to find the sys_call_table\n",MODNAME);
		return -1;
	}

#ifdef SYS_CALL_INSTALL
	cr0 = read_cr0();
	unprotect_memory();
	hacked_syscall_tbl[FIRST_NI_SYSCALL] = (unsigned long*)sys_tag_get;
	hacked_syscall_tbl[SECOND_NI_SYSCALL] = (unsigned long*)sys_tag_send;
	hacked_syscall_tbl[THIRD_NI_SYSCALL] = (unsigned long*)sys_tag_receive;
	hacked_syscall_tbl[FOURTH_NI_SYSCALL] = (unsigned long*)sys_tag_ctl;
	protect_memory();
	printk("%s: TAG_GET has been installed on %d\n",MODNAME,FIRST_NI_SYSCALL);
	printk("%s: TAG_SEND has been installed on %d\n",MODNAME,SECOND_NI_SYSCALL);
	printk("%s: TAG_RECEIVE has been installed on %d\n",MODNAME,THIRD_NI_SYSCALL);
	printk("%s: TAG_CTL has been installed on  %d\n",MODNAME,FOURTH_NI_SYSCALL);
#else
#endif
    printk("%s: ****** module correctly mounted ******\n",MODNAME);
	init_driver();
	return 0;
}

void cleanup_module(void) {

    unsigned long cr0;
    printk("%s: *** Shutting down Module ***\n",MODNAME);
    //remove driver
    cleanup_driver();

#ifdef SYS_CALL_INSTALL
	cr0 = read_cr0();
	unprotect_memory();
	hacked_syscall_tbl[FIRST_NI_SYSCALL] = (unsigned long*)hacked_ni_syscall;
	hacked_syscall_tbl[SECOND_NI_SYSCALL] = (unsigned long*)hacked_ni_syscall;
	hacked_syscall_tbl[THIRD_NI_SYSCALL] = (unsigned long*)hacked_ni_syscall;
	hacked_syscall_tbl[FOURTH_NI_SYSCALL] = (unsigned long*)hacked_ni_syscall;
	protect_memory();
#else
#endif
	printk("%s: *** Module  unmounted *** \n",MODNAME);
}
