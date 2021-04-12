#ifndef THREAD_GLO
#define THREAD_GLO

#include "Include.h"

typedef struct Thread{
    pthread_t tid;
    void* data;
}Thread;

Thread* newThread(void* (*func)(void* arg),void* arg,void* data,void(*after)(void* arg));
bool isCurrentThread(Thread* t);
int join(Thread* t);
void delThread(Thread* t,void(*after)(void* arg));
int detach(Thread* t);

#endif