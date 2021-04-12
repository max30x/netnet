#ifndef COMMON_GLO
#define COMMON_GLO

#include "Include.h"
#include "MemPool.h"

typedef struct list_node{
    struct list_node* next;
    void* arg;
}list_node;

typedef struct pair{
    void* a;
    void* b;
}pair;

typedef struct list_link{
    struct list_link *next,*prev;
}list_link;

// this should be done from the beginning of this ...
// now it's too late.this project i think it should be archived now...
// this is my last word for this bunch of code...
#define list_entry(ptr,type,member) \
    ((type *)((char*)ptr-(unsigned long)(&((type *)0)->member)))

typedef struct task_e{
    int num;
    union {
        struct {
            void (*func)();
        }Zero;
        struct {
            void (*func)(void*);
            void *arg1;
        }One;
        struct {
            void (*func)(void*,void*);
            void *arg1,*arg2;
        }Two;
        struct {
            void (*func)(void*,void*,void*);
            void *arg1,*arg2,*arg3;
        }Three;
        struct {
            void (*func)(void*,void*,void*,void*);
            void *arg1,*arg2,*arg3,*arg4;
        }Four;
    }f;
}task_e;

pair* newPair(void* a,void* b);
void delPair(pair* p);

void addNode(list_node* head,void* o);
void _addNode(list_node* head,list_node* node);
list_node* newNode(void* a);
void delList(list_node* head,void(*func)(void* arg));
list_node* at(list_node* head,int pos);
void linkNode(list_node* head,list_node* _head);
list_node* pickNode(list_node* head,int pos);

void free_taske(task_e* t);
void invokeArgs_e(task_e* t);
task_e* funcZeroArg(void(*func)());
task_e* funcOneArg(void(*func)(void*),void* arg);
task_e* funcTwoArg(void(*func)(void*,void*),void* arg1,void* arg2);
task_e* funcThreeArg(void(*func)(void*,void*,void*),void* arg1,void* arg2,void* arg3);
task_e* funcFourArg(void(*func)(void*,void*,void*,void*),void* arg1,void* arg2,void* arg3,void* arg4);

#endif