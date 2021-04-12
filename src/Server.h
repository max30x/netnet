#include "Socket.h"

typedef struct server{
    alignas(16) atomic_int open;
    int sockfd;
    addr_ipv4* addr; // bind to this
}server;

server* newServer(const char* host,uint16_t port);
server* newUdpServer(const char* host,uint16_t port,bool connectMode);
void delServer(server* server);
int getServerFd(server* server);

bool isOpen_s(server* s);