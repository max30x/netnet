#include "EpollApi.h"

epoll_api* newEpollApi(int size){
    epoll_api* api = (epoll_api*)malloc(sizeof(epoll_api));
    api->size = size;
    api->events = (ep_event*)calloc(size,sizeof(ep_event));
    api->ready = (ep_event*)calloc(size,sizeof(ep_event));
    atomic_init(&api->maxfd,0);
    api->epfd = epoll_create(1);
    return api;
}

void delEpollApi(epoll_api* e){
    close(e->epfd);
    free(e->events);
    free(e->ready);
    free(e);
}

void addEvent(epoll_api* api,int fd,int events,bool first){
    if (first && fd>=api->size){
        int ns = api->size;
        ep_event* nready = (ep_event*)calloc(2*ns,sizeof(ep_event));
        ep_event* nevents = (ep_event*)calloc(2*ns,sizeof(ep_event));
        memcpy(nready,api->ready,ns*sizeof(ep_event));
        memcpy(nevents,api->events,ns*sizeof(ep_event));
        free(api->ready);
        free(api->events);
        api->ready = nready;
        api->events = nevents;
        api->size = 2*ns;
    }
    api->events[fd].data.fd = fd;
    api->events[fd].events |= events;
    int op;
    if (first){
        op = EPOLL_CTL_ADD;
        atomic_fetch_add(&api->maxfd,1);
    } 
    else op = EPOLL_CTL_MOD;
    epoll_ctl(api->epfd,op,fd,&api->events[fd]);
}

void delEvent(epoll_api* api,int fd,int events){
    api->events[fd].events &= ~events;
    epoll_ctl(api->epfd,EPOLL_CTL_MOD,fd,&api->events[fd]);
}

void loseInterst(epoll_api* api,int fd){
    api->events[fd].events = 0;
    epoll_ctl(api->epfd,EPOLL_CTL_DEL,fd,NULL);
    atomic_fetch_sub(&api->maxfd,1);
}