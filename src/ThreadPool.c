#include "ThreadPool.h"

bool isOpen_t(threadPool* p){
    return atomic_load(&p->open)==1;
}

void* t_loop(void* p){
    threadPool* _p = (threadPool*)p;
    atomic_fetch_add(&(_p->alive),1);
    while(isOpen_t(_p)){
        task_e* t = (task_e*)poll_b(_p->q,true);
        if (t!=NULL){
            invokeArgs_e(t);
            free_taske(t);
        }
    }
    atomic_fetch_sub(&(_p->alive),1);
}

threadPool* newThreadPool(int num){
    threadPool* p = (threadPool*)malloc(sizeof(threadPool));
    p->num = num;
    atomic_init(&p->alive,0);
    atomic_init(&p->open,1);
    p->q = newBlockQueue(num);
    p->threads = (Thread**)malloc(num*sizeof(Thread*));
    for (int i=0;i<num;++i){
        p->threads[i] = newThread(t_loop,p,NULL,NULL);
    }
    for (int i=0;i<num;++i){
        detach(p->threads[i]);
    }
    return p;
}

void delThreadPool(threadPool* p){
    atomic_store(&p->open,0);
    wake_all(p->q);
    while (atomic_load(&p->alive)!=0);
    delBlockQueue(p->q);
    for (int i=0;i<p->num;++i){
        delThread(p->threads[i],NULL);
    }
    free(p);
}

bool addTask_t(threadPool* p,task_e* t){
    if (!atomic_load(&p->open)) return false;
    push_b(p->q,t);
    return true;
}

