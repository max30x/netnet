#include "Include.h"
#include "ThreadPool.h"
#include "Channel.h"
#include "Common.h"

typedef struct pro{
    int type; 
    // remember to free channel and content manually 
    // they are all allocated from the global mempool for now
    void (*onEvent)(Channel* channel,uint64_t id,uint16_t length,uchar* content);


    threadPool* pool;
}pro;

typedef struct pros{
    pro** ps;
    int num;
}pros;

pros* newPros(int num);
bool addPro(pros* ps,pro* p);

pro* newPro(int type,void(*onEvent)(Channel*,uint64_t,uint16_t,uchar*),int threadNum);
void delPro(pro* p);


