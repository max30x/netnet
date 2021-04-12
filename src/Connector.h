#include "Socket.h"

typedef struct connector{
    addr_ipv4* addr;
    int sockfd;
    bool open;
}connector;

connector* newConnector(addr_ipv4* addr);
void delConnector(connector* c);

