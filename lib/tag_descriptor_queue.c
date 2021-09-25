#include "../include/tag_descriptor_queue.h"

#define QUEUE "QUEUE"

struct available_tag_descriptor* create_queue(void)
{
    int i;
    struct available_tag_descriptor* queue = (struct available_tag_descriptor*)kzalloc(sizeof(struct available_tag_descriptor),GFP_KERNEL | GFP_NOWAIT);
    queue->capacity = MAX_TAG_SERVICES;
    queue->first = 0;
    queue->available_tags = 0;

    queue->last =  MAX_TAG_SERVICES - 1 ;
    queue->array = (int*)kzalloc(MAX_TAG_SERVICES * sizeof(int), GFP_KERNEL | GFP_NOWAIT);

    for(i = 0 ; i < MAX_TAG_SERVICES ; i++){
        enqueue(queue, i);
    }
    //alla fine available_tags = 256
    printk("Available tag at queue creation are %d\n", queue->available_tags);
    return queue;
}

// Queue is empty when there are no more available tags
int is_full(struct available_tag_descriptor* queue)
{
    return queue->available_tags == 0;
}

// Queue are available all tags descriptor
int is_empty(struct available_tag_descriptor* queue)
{
    return queue->available_tags == queue->capacity;
}

// Function to add an item to the queue. Returns nothing
// It changes rear and size
void enqueue(struct available_tag_descriptor* queue, int tag_descriptor)
{
    if (is_empty(queue))
        return;
    queue->last = (queue->last + 1)%(queue->capacity);
    queue->array[queue->last] = tag_descriptor;
    queue->available_tags = queue->available_tags + 1;
}

// Function to remove an item from queue. Returns item removed
// It changes first element and size
int dequeue(struct available_tag_descriptor* queue)
{
    int item;

    if (is_full(queue))
        return INT_MIN;
    item = queue->array[queue->first];
    queue->first = (queue->first + 1)%(queue->capacity);
    queue->available_tags = queue->available_tags - 1 ;
    return item;
}

// Function to get first element of queue
int first(struct available_tag_descriptor* queue)
{
    if (is_empty(queue))
        return INT_MIN;
    return queue->array[queue->first];
}

// Function to get last element of queue
int last(struct available_tag_descriptor* queue)
{
    if (is_empty(queue))
        return INT_MIN;
    return queue->array[queue->last];
}