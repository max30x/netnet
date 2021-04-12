#include "Include.h"
#include "Channel.h"
#include "Thread.h"
#include "Common.h"
#include "Socket.h"
#include "Server.h"
#include "BufCodec.h"
#include "Pros.h"
#include "TimeWheel.h"
#include "BlockingQueue.h"
#include "IdGenerator.h"
#include "EpollApi.h"

#define NCHANNEL 1024

#define RMASK 1<<0
#define WMASK 1<<1

#define REVENT EPOLLIN
#define WEVENT EPOLLOUT

#define CLIENTGROUP(P) initGroup(-1,P,false,NULL)
#define SERVERGROUP(P,S) initGroup(-1,P,true,S)

typedef struct eventsLoop{
    int capacity;
    atomic_int open;
    id_generator* ids;
    atomic_int size; // channel num
    Channel** channels; 
    uchar* masks; // ready ops for each channel
    int* pending;
    epoll_api* api;
    void* data; // workergroup
    block_queue* tasks;
    wheel* t_wheel;
    Thread* thread;
}eventsLoop;

typedef struct workerGroup{
    pros* _pros;
    uint64_t index;
    Thread* listener;
    Thread** threads;
    int num;
}workerGroup;

workerGroup* initGroup(int num,pros* _pros,bool isServer,server* s);
void delGroup(workerGroup* group);
void scheduleRoutine(eventsLoop* e,task_e* task,long seconds,bool again);
void addRoutine(workerGroup* g,task_e* task,long seconds,bool again);

void writeChannel(Channel* c,uint32_t type,const uchar* bytes,uint16_t length);
Channel* connTo_e(workerGroup* group,addr_ipv4* addr);
void freeChannel(Channel* c,bool on);
void syncOn(workerGroup* g);
