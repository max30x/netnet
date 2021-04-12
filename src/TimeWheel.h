#ifndef TIMEWHEEL_GLO
#define TIMEWHEEL_GLO

#include "Include.h"
#include "Common.h"

typedef struct timeval time_out;

typedef struct tnode{
    task_e* task;
    bool again;
    time_out _time;
}tnode;

typedef struct wheel{
    int slots;
    time_out _free;  
    struct chain** chains;
}wheel;

typedef struct chain{
    time_out t;
    list_node* nodes;
    struct wheel* w;
    bool empty;
}chain;

tnode* newTNode(task_e* task,long seconds,bool again);

chain* newChain(wheel* w,long s);
void delChain(chain* c);

void addTNode(wheel* w,tnode* n);
wheel* newWheel(int slots);
void delWheel(wheel* w);
void addNewEvent_w(wheel* w,task_e* task,long seconds,bool again);

long timePass(time_out* clock);
void updateFreeTime(wheel* w);

void execute_t(wheel* w,time_out* clock);

#endif