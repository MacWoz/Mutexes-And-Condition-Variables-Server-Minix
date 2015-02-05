#ifndef _QUEUE_H
#define _QUEUE_H

#include "inc.h"

typedef struct Node{
    endpoint_t proc_nr;
    struct Node* next;
} Node;

typedef struct queue {
    int mutex_id;
    Node* head;
    Node* tail;
} queue;

queue* createQueue(int mutex_id);

void enqueue(endpoint_t proc_nr, queue* Q);

int pop(queue* Q);

void remove_from_queue(queue* Q, Node* node);

int isEmpty(queue* Q);

void destroyQueue(queue* Q);

#endif
