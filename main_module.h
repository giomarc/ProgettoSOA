#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/apic.h>
#include <linux/syscalls.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/hashtable.h>
#include <linux/types.h> 
#include <linux/rculist.h>
#include "include/configure.h"
#include "include/custom_structures.h"
#include "include/custom_errors.h"
#include "./include/vtpmo.h"
#include "./include/sys_call_table_discovery.h"
#include "./include/tag_descriptor_queue.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Giorgia Marchesi");
MODULE_DESCRIPTION("Progetto SOA");

//STAFF FOR SYSCALL TABLE
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,3,0)
    #include <asm/switch_to.h>
#else
    #include <asm/system.h>
#endif
#ifndef X86_CR0_WP
#define X86_CR0_WP 0x00010000
#endif

#define MAX_FREE 15
#define MAX_SIZE 8192
#define SYS_CALL_INSTALL

//DRIVER
#define DEVICE_NAME "miodev"
#define MINORS 8
#define OBJECT_MAX_SIZE  (4096)
#define MAX_LINE 60
//DRIVER IS ASSUMED SINGLE_SESSION_OBJECT


extern unsigned long *hacked_ni_syscall;
extern unsigned long **hacked_syscall_tbl;
extern unsigned long sys_call_table_address;
extern unsigned long sys_ni_syscall_address;
extern int good_area(unsigned long * addr);
extern int validate_page(unsigned long *addr);
extern void syscall_table_finder(void);
extern struct tag_service *search_tag_by_tagdescriptor(int tag);
extern struct tag_service *search_tag_by_key(int key,int hash_key,int command);

// Struttura Driver
typedef struct _object_state{
    struct mutex object_busy;
    struct mutex operation_synchronizer;
    int valid_bytes;
    char * stream_content;
} object_state;

//HASH TABLE
DEFINE_HASHTABLE(tbl,9);
EXPORT_SYMBOL(tbl);

struct available_tag_descriptor *tag_queue;

int myhash(const int s) {
    int key = 0;

    //if (s == IPC_PRIVATE_KEY)
    //    key = 255;
    //else
    //      key = s % 255;
    key = s % 256;
    return key;
}

//inizializza il singolo livello
void init_single_level(struct level *level_ptr, int level_id){

    level_ptr->id = level_id;
    level_ptr->flag = 0;
    level_ptr->msg = NULL;
    rwlock_init(&level_ptr->lock);
    level_ptr->sleepers = 0;
    init_waitqueue_head(&level_ptr->wait_queue);
    INIT_LIST_HEAD(&(level_ptr->reader_sleepers));
}


// inizializza tutti i livelli di un tag service
void init_levels(struct tag_service *tag_service)
{
    int i;
    for( i = 0; i < MAX_LEVELS; i++){
        init_single_level(&(tag_service->levels[i]), i);
    }
}

int check_access_perm(struct tag_service *tagService)
{
    if(tagService->permission == TAG_ACCESS_PRIVATE && (int)current->tgid != tagService->owner){
        return ER_ACCESS;
    }
    return 0;
}

//crea il tag service inizializzando con key, permission, e i 32 livelli
int create_tag_service(struct tag_service *tag, int key, int permission)
{
    int key_tag;
    int tag_desc;

    tag_desc = dequeue(tag_queue);
    tag->tag_descriptor = tag_desc;
    tag->key = key;
    tag->owner = (int) current->tgid;
    tag->permission = permission;
    rwlock_init(&tag->lock);
    init_levels(tag);

    key_tag = myhash(key);
    hash_add_rcu(tbl, &(tag->node), key_tag);
    printk("%s: TAG SERVICE CREATO, tag descriptor: %d, owner: %d, key: %d in key_a: %d, permission: %d \n",MODNAME, tag->tag_descriptor, tag->owner, tag->key, key_tag, tag->permission);

    return tag->tag_descriptor;
}


int check_readers(struct tag_service * tag_service)
{
    int res = 0;
    int i;

    for(i = 0; i < MAX_LEVELS; i++){
        if(tag_service->levels[i].sleepers > 0){
            return -1;
        }
    }
    return res;
}


void remove_thread_data(struct level *tag_level)
{
    struct thread_data *cursor, *temp;

    list_for_each_entry_safe(cursor, temp, &(tag_level->reader_sleepers), list){
        struct thread_data *p;
        p = cursor;
        list_del_rcu(&p->list);
        synchronize_rcu();
        kfree(p);
    }
}


//cerco il tag_service attraverso il tag_descriptor
struct tag_service *search_tag_by_tagdescriptor(int tag){

    struct tag_service *cur = NULL;
    unsigned bkt;
    hash_for_each_rcu(tbl, bkt, cur, node){
        if(cur->tag_descriptor == tag){
            return cur;
        }
    }
    return NULL;
}



struct tag_service* search_tag_by_key(int key,int hash_key,int command){

    struct tag_service *tag_to_find = NULL;
    hash_for_each_possible_rcu(tbl, tag_to_find, node, hash_key) {
        if (tag_to_find->key == key) {
            return tag_to_find;
        }
    }
    return NULL;
}



//tag_ctl wake up dei thread sleepers
int wake_up_all_level(struct tag_service *tag){

    int i, readers, prev_val;
    struct level *tag_level;
    struct thread_data *cursor, *temp;
    int woke_up = 0;
    char *wake_up_message = "AWAKE_ALL woke me up!"; //messaggio di default

    for(i = 0; i < MAX_LEVELS; i++){
        tag_level = &(tag->levels[i]);
        readers = __atomic_load_n( &tag_level->sleepers, __ATOMIC_SEQ_CST );

        if(readers > 0){

            ( tag_level->msg )->text = kzalloc(strlen(wake_up_message), GFP_KERNEL);
            if((tag_level->msg)->text == NULL){
                printk("%s, TAG_CTL (AWAKE_ALL): error allocating wake_up message\n",MODNAME);
                return ER_ALLOC;
            }

            write_lock(&tag_level->lock);
            memcpy((char *)(tag_level->msg)->text, (char*) wake_up_message, strlen(wake_up_message));
            write_unlock(&tag_level->lock);
            (tag_level->msg)->msg_len = (int) strlen((tag_level->msg)->text);
            __atomic_exchange( &(tag_level->flag), &readers, &prev_val, __ATOMIC_SEQ_CST);

            list_for_each_entry_safe(cursor, temp, &(tag_level->reader_sleepers), list){
                wake_up_process(cursor->t);
            }

            remove_thread_data(tag_level);
            printk("%s: WOKE UP tag %d level %d\n", MODNAME, tag->tag_descriptor, tag_level->id);
            woke_up++;
        }
    }
    return woke_up;
}




