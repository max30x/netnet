#include "BufCodec.h"

void fill(Codec* c,uchar* bytes,int length){
    c->pos = 0;
    c->size = length;
    c->buf = bytes;
}

bool checkSize(Codec* c,int n){
    return c->pos+n-1<=c->size;
}

int _writeInt16(Codec*c,uint16_t num,bool raw){
    uchar* buf = c->buf;
    if (!checkSize(c,2)) return -1;
    uint16_t _num = (!raw)?htons(num):num;
    for (int i=0,k=8;i<2;++i,k-=8){
        buf[c->pos++] = (_num>>k)&0xff;
    }
    return 0;
}

int writeRawInt16(Codec*c,uint16_t num){
    return _writeInt16(c,num,true);
}

int writeInt64(Codec* c,uint64_t num){
    uint32_t a = (uint32_t)((num>>32)&0xffffffff);
    uint32_t b = (uint32_t)(num&0xffffffff);
    if (writeInt32(c,a)==-1) return -1;
    if (writeInt32(c,b)==-1) return -1;
    return 0;
}

// use network byte order
int writeInt32(Codec* c,uint32_t num){
    uchar* buf = c->buf;
    if (!checkSize(c,4)) return -1;
    uint32_t _num = htonl(num);
    for (int i=0,k=24;i<4;++i,k-=8){
        buf[c->pos++] = (_num>>k)&0xff;
    }
    return 0;
}

int writeInt16(Codec* c,uint16_t num){
    return _writeInt16(c,num,false);
}

uint64_t readInt64(Codec* c,int* sign){
    uint64_t a = 0;
    uint32_t pa = readInt32(c,sign);
    if (*sign==-1) return 0;
    uint32_t pb = readInt32(c,sign);
    if (*sign==-1) return 0;
    a |= (uint64_t)pa<<32;
    a |= pb;
    return a;
}

uint32_t readInt32(Codec* c,int* sign){
    uchar* buf = c->buf;
    if (!checkSize(c,4)){
        if (sign!=NULL) *sign = -1;
        return 0;
    }
    uint32_t n = 0;
    for (int i=0,k=24;i<4;++i,k-=8){
        n |= buf[c->pos++]<<k;
    }
    return ntohl(n);
}

uint16_t _readInt16(Codec* c,int* sign,bool raw){
    uchar* buf = c->buf;
    if (!checkSize(c,2)){
        if (sign!=NULL) *sign = -1;
        return 0;
    }
    uint16_t n = 0;
    for (int i=0,k=8;i<2;++i,k-=8){
        n |= buf[c->pos++]<<k;
    }
    if (!raw) return ntohs(n);
    return n;
}

uint16_t readInt16(Codec* c,int* sign){
    return _readInt16(c,sign,false);
}

uint16_t readRawInt16(Codec* c,int* sign){
    return _readInt16(c,sign,true);
}

int writeBytes(Codec* c,const uchar* bytes,int n){
    uchar* buf = c->buf;
    if (!checkSize(c,n)) return -1;
    for (int i=0;i<n;++i){
        buf[c->pos++] = bytes[i];
    }
    return 0;
}

void readBytes(Codec* c,int len,uchar* bytes,int* sign){
    uchar* buf = c->buf;
    if (!checkSize(c,len)){
        if (sign!=NULL) *sign = -1;
        return;
    }
    memcpy(bytes,&buf[c->pos],len);
    c->pos += len;
}

void reset(Codec* c){
    c->pos = 0;
}

Codec* newCodec(){
    Codec* c = (Codec*)malloc(sizeof(Codec));
    return c;
}

void delCodec(Codec* c){
    free(c);
}

int firstDelimiter(const uchar* buf,int length){
    int i=0,s=0;
    while (i<length){
        if (buf[i]=='\\'){
            if (i+3<length && buf[i+1]=='r' && buf[i+2]=='\\' && buf[i+3]=='n'){
                s = 1;
                break;
            }
        }
        ++i;
    }
    return (s==1) ? i : -1;
}

int firstMagic(const uchar* buf,int pos,int length){
    int i=0,s=0;
    while (i<length){
        if (i+1<length && buf[pos-i]==(uchar)0xca && buf[pos-i-1]==(uchar)0xca){
            s = 1;
            break;
        }
        ++i;
    }
    return (s==1) ? pos-i-1 : -1;
}