#ifndef SOCKET_GLO
#define SOCKET_GLO

#include "Include.h"

typedef struct sockaddr_in addr_ipv4;

typedef struct Option{
    bool isTCP;
    bool isNonBlock;
    bool reuseaddr;
    bool keepalive;
    bool nodelay;
    bool fastopen;
}Option;

Option* newOption();
void delOption(Option* o);
addr_ipv4* newAddr(const char* host,uint16_t port);
void delAddr(addr_ipv4* addr);
int newTcpSocket(Option* o);
int newUdpSocket(bool connected);
int connectTo(int sockfd,addr_ipv4* a);
int bindWith(int sockfd,addr_ipv4* a);
int acceptConnection(int sockfd,addr_ipv4* remoteAddr,socklen_t* size,Option* o);
int readUdpSocket(int fd,const char* msg,int length,int* sign,addr_ipv4* addr);
int readSocket(int fd,struct msghdr *msg,int length,int* sign);
int writeUdpSocket(int fd,const char* buf,int length,int* sign,addr_ipv4* addr);
int writeSocket(int fd,const char* buf,int length,int* sign);
int stopRead(int fd);
int stopWrite(int fd);

#endif