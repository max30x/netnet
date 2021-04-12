#include "Connector.h"

connector* newConnector(addr_ipv4* addr){
    connector* c = (connector*)malloc(sizeof(connector));
    Option o;
    o.fastopen = true;
    o.isNonBlock = true;
    o.isTCP = true;
    o.keepalive = true;
    o.nodelay = true;
    int sockfd = newTcpSocket(&o);
    c->sockfd = sockfd;
    c->addr = addr;
    if (connectTo(sockfd,addr)<0){
        delConnector(c);
        return -1;
    }
    return c;
}

void delConnector(connector* c){
    close(c->sockfd);
    delAddr(c->addr);
    free(c);
}
