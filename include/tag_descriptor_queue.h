#ifndef TAG_DESCRIPTOR_QUEUE_H
#define TAG_DESCRIPTOR_QUEUE_H

#include "configure.h"
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include "custom_structures.h"
#include "custom_errors.h"



struct available_tag_descriptor* create_queue(void);
int is_full(struct available_tag_descriptor* queue);
int is_empty(struct available_tag_descriptor* queue);
void enqueue(struct available_tag_descriptor* queue, int tag_descriptor);
int dequeue(struct available_tag_descriptor* queue);
int first(struct available_tag_descriptor* queue);
int last(struct available_tag_descriptor* queue);

typedef struct available_tag_descriptor{
    int first, last, available_tags;
    int capacity; // 256
    int *array;
}available_tag_descriptor;

#endif