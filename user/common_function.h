#ifndef COMMON_FUNCTION_H
#define COMMON_FUNCTION_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include "../include/configure.h"
#include "../include/custom_errors.h"

#define TAG_GET 134
#define TAG_SEND 174
#define TAG_RECEIVE 177
#define TAG_CTL 178


int remove_tag(int tag);
int wakeup_tag(int tag);
int open_tag(int key);
int create_tag(int key, int permission);
int send_msg(int tag, int level, char *msg, unsigned int thread_id);
int receive_msg(int tag, int level, char *msg, unsigned int thread_id);



#endif
