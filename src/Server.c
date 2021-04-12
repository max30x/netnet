#include "Server.h"

server* newServer0(const char* host,uint16_t port,bool isTcp,bool connectMode){
    addr_ipv4* addr = newAddr(host,port);
    server* s = (server*)malloc(sizeof(server));
    int sockfd;
    if (isTcp){
        Option* o = newOption();
        o->isTCP = true;
        o->reuseaddr = true;
        sockfd = newTcpSocket(o);
        delOption(o);
    }else{
        sockfd = newUdpSocket(connectMode);
    }
    s->sockfd = sockfd;
    s->addr = addr;
    atomic_init(&s->open,1);
    if (bindWith(sockfd,addr)<0){
        delServer(s);
        return NULL;
    }
    if (isTcp) listen(sockfd,512);
    return s;
}

server* newUdpServer(const char* host,uint16_t port,bool connectMode){
    return newServer0(host,port,false,connectMode);
}

server* newServer(const char* host,uint16_t port){
    return newServer0(host,port,true,false);
}

bool isOpen_s(server* s){
    return atomic_load(&s->open)==1;
}

int getServerFd(server* server){
    return server->sockfd;
}

void delServer(server* server){
    atomic_store(&server->open,0);
    close(server->sockfd);
    delAddr(server->addr);
    free(server);
}
