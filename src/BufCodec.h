#ifndef BUFCODEC_GLO
#define BUFCODEC_GLO

#include "Include.h"

typedef struct Codec{
    uchar* buf;
    int size; // buf size
    int pos; // next op will happen here
}Codec;

Codec* newCodec();
void delCodec(Codec* c);

void fill(Codec* c,uchar* bytes,int length);
uint64_t readInt64(Codec* c,int* sign);
uint32_t readInt32(Codec* c,int* sign);
uint16_t readInt16(Codec* c,int* sign);
uint16_t readRawInt16(Codec*c,int* sign);
int writeInt64(Codec* c,uint64_t num);
int writeInt32(Codec* c,uint32_t num);
int writeRawInt16(Codec*c,uint16_t);
int writeInt16(Codec* c,uint16_t num);
int writeBytes(Codec* c,const uchar* bytes,int n);
void readBytes(Codec* c,int len,uchar* buf,int* sign);
void reset(Codec* c);
int firstDelimiter(const uchar* buf,int length); // find \r\n
int firstMagic(const uchar* buf,int pos,int length); // find 0xcaca (magic)

#endif