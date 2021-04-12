#ifndef BLOCKQUEUE_GLO
#define BLOCKQUEUE_GLO

#include "Include.h"
#include "Common.h"

typedef struct Node{
    struct Node* next;
    void* val;
}Node;

typedef struct block_queue{
    int bufnum;
    Node** bufs;
    pthread_mutex_t** locks;
    pthread_cond_t** conds;
    atomic_int wake_up; // set this to 1 when this queue is about to be deleted
    atomic_int write_pos;
    atomic_int read_pos;
    atomic_int ref;
}block_queue;

block_queue* newBlockQueue(int bufnum);
void delBlockQueue(block_queue* q);

void push_b(block_queue* q,void* val);
void* poll_b(block_queue* q,bool block);
void wake_all(block_queue* q);
bool isEmpty_b(block_queue* q);

#endif