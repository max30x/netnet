#include "Socket.h"

Option* newOption(){
    Option* o = (Option*)malloc(sizeof(Option));
    o->fastopen = false;
    o->isNonBlock = false;
    o->isTCP = false;
    o->keepalive = false;
    o->nodelay = false;
    o->reuseaddr = false;
    return o;
}

void delOption(Option* o){
    free(o);
}

addr_ipv4* newAddr(const char* host,uint16_t port){
    addr_ipv4* a = (addr_ipv4*)malloc(sizeof(addr_ipv4));
    a->sin_family = AF_INET;
	inet_pton(AF_INET,host,(void*)&(a->sin_addr));
	a->sin_port = htons(port);
    return a;
}

void delAddr(addr_ipv4* addr){
    free(addr);
}

void setOpt(int sockfd,Option* o){
    if (o->reuseaddr)
        setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&o->reuseaddr,sizeof(bool));
    if (o->keepalive)
        setsockopt(sockfd,SOL_SOCKET,SO_KEEPALIVE,&o->keepalive,sizeof(bool));
    if (o->isTCP){
        if (o->nodelay)
            setsockopt(sockfd,SOL_SOCKET,TCP_NODELAY,&o->nodelay,sizeof(bool));
        if (o->fastopen)
            setsockopt(sockfd,SOL_SOCKET,TCP_FASTOPEN,&o->fastopen,sizeof(bool));
    }
}

int newTcpSocket(Option* o){
    int flag = SOCK_STREAM;
    if (o->isNonBlock) flag |= O_NONBLOCK;
    int sockfd = socket(AF_INET,flag,0);
    setOpt(sockfd,o);
    return sockfd;
}

int newUdpSocket(bool connected){
    int flag = (connected) ? SOCK_SEQPACKET : SOCK_DGRAM;
    return socket(AF_INET,SOCK_DGRAM,0);
}

int connectTo(int sockfd,addr_ipv4* a){
    return connect(sockfd,(struct sockaddr*)a,sizeof(addr_ipv4));
}

int bindWith(int sockfd,addr_ipv4* a){
    return bind(sockfd,(struct sockaddr*)a,sizeof(addr_ipv4));
}

// accept connection then turn it into nonblocking mode
int acceptConnection(int sockfd,addr_ipv4* remoteAddr,socklen_t* size,Option* o){
    int fd = accept(sockfd,(struct sockaddr*)remoteAddr,size);
    if (fd==-1) return fd;
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    if (o!=NULL) setOpt(fd,o);
    return fd;
}

// some udp stuff

int readSocket0(int fd,void *msg,int length,int* sign,bool isTcp,addr_ipv4* addr){
    int tmp=0,times=0;
    while (length>0){
        int n;
        if (isTcp){
            struct msghdr *_msg = (struct msghdr*)msg;
            n = recvmsg(fd,msg,0);
        }else{
            const char *_msg = (const char*)msg;
            int len = sizeof(addr_ipv4);
            n = recvfrom(fd,_msg,length,0,addr,&len);
        }
        if (n==0){
            *sign = 2;
            return tmp;
        }
        else if (n==-1){
            if (errno==EAGAIN){
                ++times;
                if (times<5) continue;
            }
            if (tmp==0 && sign!=NULL) *sign = -1;
            return tmp;
        }
        else times = 0;
        length -= n;
        tmp += n;
        if (!isTcp) break;
    }
    if (sign!=NULL) *sign = 1;
    return tmp;
}

int readSocket(int fd,struct msghdr *msg,int length,int* sign){
    return readSocket0(fd,msg,length,sign,true,NULL);
}

int readUdpSocket(int fd,const char* msg,int length,int* sign,addr_ipv4* addr){
    return readSocket0(fd,msg,length,sign,false,addr);
}

int writeSocket0(int fd,const char* buf,int length,int* sign,bool isTcp,addr_ipv4* addr){
    int tmp = 0;
    while (length>0){
        int n = (isTcp)?send(fd,buf,length,0):sendto(fd,buf,length,0,addr,sizeof(addr_ipv4));
        if (n==-1 && errno!=EAGAIN){
            if (sign!=NULL) *sign = -1;
            return tmp;
        }
        length -= n;
        tmp += n;
    }
    if (sign!=NULL) *sign = 1;
    return tmp;
}

int writeUdpSocket(int fd,const char* buf,int length,int* sign,addr_ipv4* addr){
    return writeSocket0(fd,buf,length,sign,false,addr);
}

int writeSocket(int fd,const char* buf,int length,int* sign){
    return writeSocket0(fd,buf,length,sign,true,NULL);
}

int stopRead(int fd){
    return shutdown(fd,SHUT_RD);
}

int stopWrite(int fd){
    return shutdown(fd,SHUT_WR);
}
