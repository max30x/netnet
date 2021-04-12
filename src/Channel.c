#include "Channel.h"

charBuf* newCharBuf(int size){
    charBuf* buf = (charBuf*)malloc(sizeof(charBuf));
    buf->size = size;
    buf->pos = 0;
    buf->buf = (uchar*)malloc(size*sizeof(uchar));
    return buf;
}

void delCharBuf(charBuf* buf){
    free(buf->buf);
    free(buf);
}

void retainChannel(Channel* c){
    retain_mem(c);
    retain_mem(c->conn);
}

bool isConnectionOpen(Connection* conn){
    return atomic_load(&conn->open)==(OPEN);
}

bool isChannelOpen(Channel* c){
    return isConnectionOpen(c->conn);
}

Connection* newConnection(int sockfd,void(*read)(void*),void(*write)(void*)){
    Connection* c = (Connection*)MEMALLOC(sizeof(Connection));
    c->sockfd = sockfd;
    c->read = read;
    c->write = write;
    atomic_init(&c->open,(OPEN));
    return c;
}

void delConnection(Connection* conn){
    int fd = (isConnectionOpen(conn))?conn->sockfd:-1;
    if (DEMEMALLOC((void*)conn) && fd!=-1) close(fd);
}

Channel* newChannel(Connection* conn,void* data){
    Channel* c = (Channel*)MEMALLOC(sizeof(Channel));
    c->conn = conn;
    c->readBuf = newCharBuf((BUFSIZE));
    c->writeBuf = newCharBuf(2*(BUFSIZE));
    c->data = data;
    c->last_read = -1;
    return c;
}

void delChannel(Channel* c,void(*after)(void*)){
    charBuf* wb = c->writeBuf;
    charBuf* rb = c->readBuf;
    void* data = c->data;
    delConnection(c->conn);
    if (DEMEMALLOC((void*)c)){
        if (after!=NULL) after(data);
        delCharBuf(wb);
        delCharBuf(rb);
    }
}

void closeChannel(Channel* c){
    atomic_store(&c->conn->open,(CLOSE));
}

bool writeToBuf(Channel* c,uint64_t msgid,uint32_t type,const uchar* bytes,uint16_t length){
    charBuf* _buf = c->writeBuf;
    int pos = _buf->pos;
    int size = _buf->size;
    Codec codec;
    if (pos+length>=size){
        if (pos+length>=(MAXBUFSIZE)) return false;
        uchar* tmp = (uchar*)malloc((pos+length+18)*sizeof(uchar));
        memcpy(tmp,_buf->buf,pos);
        free(c->writeBuf->buf);
        c->writeBuf->buf = tmp;
        c->writeBuf->size = pos+length+18;
    }
    fill(&codec,(_buf->buf)+(_buf->pos),14+length+4);
    writeRawInt16(&codec,(uint16_t)(MAGIC));
    writeInt64(&codec,msgid);
    writeInt16(&codec,type);
    writeInt16(&codec,length);
    if (length>0) writeBytes(&codec,bytes,length);
    writeBytes(&codec,"\\r\\n",4);
    c->writeBuf->pos += 14+length+4;
    return true;
}