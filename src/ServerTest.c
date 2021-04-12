#include "EventsLoop.h"

void sendNum(Channel* channel,uint64_t id,uint16_t length,uchar* content){
    int* pint = (int*)malloc(sizeof(int));
    *pint = *((int*)content)+2;
    scheduleRoutine(channel->data,funcFourArg(writeChannel,channel,0,pint,sizeof(int)),2,false);
}

int main(){
    pros* ps = newPros(5);
    pro* p = newPro(0,sendNum,2);
    addPro(ps,p);
    server* ser = newServer("127.0.0.1",13451);

    workerGroup* g = SERVERGROUP(ps,ser);

    syncOn(g);
    return 0;
}

#if 0
int main(){
    server* ser = newUdpServer("127.0.0.1",13455,false);
    int fd = getServerFd(ser);
    char buf[128];
    memset(&buf,0,sizeof(buf));
    addr_ipv4 client;
    int got = readUdpSocket(fd,buf,128,NULL,&client);
    for (int i=0;i<got;++i) printf("%c",buf[i]);
    writeUdpSocket(fd,buf,got,NULL,&client);

    delServer(ser);
    return 0;
}
#endif
