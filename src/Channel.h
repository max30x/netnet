#ifndef CHANNEL_GLO
#define CHANNEL_GLO

#include "Include.h"
#include "BufCodec.h"
#include "Socket.h"
#include "MemPool.h"

#define MAXBUFSIZE 65537
#define BUFSIZE 4*1024
#define MAGIC 0xcaca

#define OPEN 1
#define CLOSE 0

typedef struct charBuf{
    uchar* buf;
    int pos;
    int size;
}charBuf;

charBuf* newCharBuf(int size);
void delCharBuf(charBuf* buf);

typedef struct Connection{
    void (*write)(void* args);
    void (*read)(void* args);
    int sockfd;
    atomic_int open;
}Connection;

Connection* newConnection(int sockfd,void(*read)(void*),void(*write)(void*));
void delConnection(Connection* conn);

typedef struct Channel{
    void* data; // eventsLoop
    charBuf* writeBuf;
    charBuf* readBuf;
    Connection* conn;
    time_t last_read;
    long id;
    addr_ipv4 addr;
}Channel;

void delChannel(Channel* c,void(*after)());
Channel* newChannel(Connection* conn,void* data);
bool isChannelOpen(Channel* c);
void closeChannel(Channel* c);
void retainChannel(Channel* c);

bool writeToBuf(Channel* c,uint64_t msgid,uint32_t type,const uchar* bytes,uint16_t length);

#endif