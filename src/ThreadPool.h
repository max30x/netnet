#ifndef THREADPOOL_GLO
#define THREADPOOL_GLO

#include "Include.h"
#include "Common.h"
#include "BlockingQueue.h"
#include "Thread.h"

typedef struct threadPool{
    atomic_int open;
    int num;
    atomic_int alive;
    Thread** threads;
    block_queue* q;
}threadPool;

threadPool* newThreadPool(int num);
void delThreadPool(threadPool* pool);

bool addTask_t(threadPool* p,task_e* t);

#endif
