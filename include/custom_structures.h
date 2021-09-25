#ifndef CUSTOM_STRUCTURES_H
#define CUSTOM_STRUCTURES_H

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/hashtable.h>
#include <linux/types.h>
#include <linux/rculist.h>
#include "configure.h"

//struttura messaggio
typedef struct message{
    char *text;
    unsigned msg_len;
    unsigned readers;
}message;

//struttura thread_data
typedef struct thread_data{
    struct list_head list;
    struct task_struct *t;
    struct message **msg_ptr_addr;
}thread_data;

//struttura livello
typedef struct level{
    int id;
    int flag;
    struct message *msg;
    rwlock_t lock;
    unsigned sleepers; //numero di readers in attesa
    wait_queue_head_t wait_queue;
    struct list_head reader_sleepers; //lista thread_data = reader in attesa

}level;

//h_node hash table node
typedef struct tag_service{
    int key;
    int tag_descriptor;
    int permission;
    int owner;
    rwlock_t lock;
    struct level levels[MAX_LEVELS];
    struct hlist_node node;
}tag_service;

#endif