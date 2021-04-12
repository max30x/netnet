#include "Common.h"

void addNode(list_node* head,void* o){
    list_node* node = newNode(o);
    _addNode(head,node);
}

void _addNode(list_node* head,list_node* node){
    list_node* p = head;
    if (p->arg==NULL){
        p->arg = node->arg;
        free(node);
        return;
    }
    while (p->next!=NULL){
        p = p->next;
    }
    p->next = node;
}

list_node* newNode(void* a){
    list_node* node = (list_node*)malloc(sizeof(list_node));
    node->next = NULL;
    node->arg = a;
    return node;
}

void delList(list_node* head,void (*func)(void* arg)){
    list_node* t;
    while (head!=NULL){
        t = head->next;
        if (func!=NULL) func(head->arg);
        else free(head->arg);
        free(head);
        head = t;
    }
}

list_node* at(list_node* head,int pos){
    list_node* p = head;
    int i = 0;
    for (i=1;i<pos;++i){
        if (p==NULL) return NULL;
        p = p->next;
    }
    return p;
}

list_node* pickNode(list_node* head,int pos){
    if (pos==0) return head;
    list_node* n = at(head,pos-1);
    if (n==NULL || n->next==NULL) return NULL;
    list_node* ne = n->next;
    n->next = ne->next;
    return ne;
}

pair* newPair(void* a,void* b){
    pair* p = (pair*)malloc(sizeof(pair));
    p->a = a;
    p->b = b;
    return p;
}

void delPair(pair* p){
    free(p);
}

void linkNode(list_node* head,list_node* _head){
    list_node* tmp = head;
    while (tmp->next!=NULL) tmp = tmp->next;
    tmp->next = _head;
}

task_e* funcZeroArg(void(*func)()){
    task_e* e = (task_e*)MEMALLOC(sizeof(task_e));
    e->num = 0;
    e->f.Zero.func = func;
    return e;
}

task_e* funcOneArg(void(*func)(void*),void* arg){
    task_e* e = (task_e*)MEMALLOC(sizeof(task_e));
    e->num = 1;
    e->f.One.func = func;
    e->f.One.arg1 = arg;
    return e;
}

task_e* funcTwoArg(void(*func)(void*,void*),void* arg1,void* arg2){
    task_e* e = (task_e*)MEMALLOC(sizeof(task_e));
    e->num = 2;
    e->f.Two.func = func;
    e->f.Two.arg1 = arg1;
    e->f.Two.arg2 = arg2;
    return e;
}

task_e* funcThreeArg(void(*func)(void*,void*,void*),void* arg1,void* arg2,void* arg3){
    task_e* e = (task_e*)MEMALLOC(sizeof(task_e));
    e->num = 3;
    e->f.Three.func = func;
    e->f.Three.arg1 = arg1;
    e->f.Three.arg2 = arg2;
    e->f.Three.arg3 = arg3;
    return e;
}

task_e* funcFourArg(void(*func)(void*,void*,void*,void*),void* arg1,void* arg2,void* arg3,void* arg4){
    task_e* e = (task_e*)MEMALLOC(sizeof(task_e));
    e->num = 4;
    e->f.Four.func = func;
    e->f.Four.arg1 = arg1;
    e->f.Four.arg2 = arg2;
    e->f.Four.arg3 = arg3;
    e->f.Four.arg4 = arg4;
    return e;
}

void invokeArgs_e(task_e* t){
    switch (t->num){
        case 0:
            t->f.Zero.func();
            break;
        case 1:
            t->f.One.func(t->f.One.arg1);
            break;
        case 2:
            t->f.Two.func(t->f.Two.arg1,t->f.Two.arg2);
            break;
        case 3:
            t->f.Three.func(t->f.Three.arg1,t->f.Three.arg2,t->f.Three.arg3);
            break;
        case 4:
            t->f.Four.func(t->f.Four.arg1,t->f.Four.arg2,t->f.Four.arg3,t->f.Four.arg4);
            break;
    }
}

void inline free_taske(task_e* t){
    DEMEMALLOC((void*)t);
}


pthread_cond_t _cond;
pthread_mutex_t _mtx;
void _wake(int sig){
    pthread_cond_broadcast(&_cond);
}

void sync(){
    struct sigaction act;
    act.sa_handler = _wake;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGINT,&act,NULL);
    pthread_cond_init(&_cond,NULL);
    pthread_mutex_init(&_mtx,NULL);
    pthread_mutex_lock(&_mtx);
    pthread_cond_wait(&_cond,&_mtx);
    pthread_mutex_unlock(&_mtx);
}
