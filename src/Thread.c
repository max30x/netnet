#include "Thread.h"

bool isCurrentThread(Thread* t){
    return pthread_self() == t->tid;
}

int join(Thread* t){
    return pthread_join(t->tid,NULL);
}

void delThread(Thread* t,void(*after)(void* arg)){
    if (after!=NULL) after(t->data);
    free(t);
}

Thread* newThread(void* (*func)(void*),void* arg,void* data,void(*after)(void* arg)){
    Thread* t = (Thread*)malloc(sizeof(Thread));
    t->data = data;
    if (pthread_create(&t->tid,NULL,func,arg)!=0){
        delThread(t,after);
        return NULL;
    }
    return t;
}

int detach(Thread* t){
    return pthread_detach(t->tid);
}