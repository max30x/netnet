#ifndef EPOLLAPI_GLO
#define EPOLLAPI_GLO

#include "Include.h"

#define RE 0 // read-event
#define WE 1 // write-event
#define ERR 2

typedef struct epoll_event ep_event;

typedef struct epoll_api{
    int epfd;
    atomic_int maxfd;
    ep_event* events;
    ep_event* ready;
    int size;
}epoll_api;

epoll_api* newEpollApi(int size);
void delEpollApi(epoll_api* e);
void addEvent(epoll_api* api,int fd,int events,bool first);
void delEvent(epoll_api* api,int fd,int events);
void loseInterst(epoll_api* api,int fd);

#endif