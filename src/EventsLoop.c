#include "EventsLoop.h"

bool inLoop(eventsLoop* e){
    return pthread_self() == e->thread->tid;
}

void startLoop_e(eventsLoop* e){
    atomic_store(&e->open,1);
}

bool isOpen_e(eventsLoop* e){
    return atomic_load(&e->open)==1;
}

void _shutdown_e(eventsLoop* e){
    atomic_store(&e->open,0);
}

void shutdown_e(eventsLoop* e){
    if (inLoop(e)) _shutdown_e(e);
    else{
        task_e* t = funcOneArg(_shutdown_e,e);
        push_b(e->tasks,t);
    }
}

bool serverSide(eventsLoop* e){
    return ((workerGroup*)e->data)->listener!=NULL;
}

eventsLoop* newEventsLoop(uint8_t index){
    eventsLoop* e = (eventsLoop*)malloc(sizeof(eventsLoop));
    e->capacity = (NCHANNEL);
    e->api = newEpollApi((NCHANNEL));
    e->channels = (Channel**)malloc((NCHANNEL)*sizeof(Channel*));
    for (int i=0;i<(NCHANNEL);++i) e->channels[i] = NULL;
    e->masks = (unsigned char*)calloc((NCHANNEL),sizeof(unsigned char));
    atomic_init(&e->size,0);
    e->pending = (int*)calloc((NCHANNEL),sizeof(int));
    e->t_wheel = newWheel(60);
    e->tasks = newBlockQueue(6);
    e->ids = newIdGenerator(index);
    atomic_init(&e->open,0);
    return e;
}

void _unreg(Channel* c,bool again){
    eventsLoop* e = (eventsLoop*)c->data;
    closeChannel(c);
    int sockfd = c->conn->sockfd;
    loseInterst(e->api,sockfd);
    close(sockfd);
    //e->channels[sockfd] = NULL;
    e->masks[sockfd] = 0;
    if (again) c->conn->read(c);
    delChannel(c,NULL);
    atomic_fetch_sub(&e->size,1);
}

void _freeChannel(Channel* c,bool on){
    eventsLoop* e = (eventsLoop*)c->data;
    if (isChannelOpen(c)){
        _unreg(c,on);
        int sockfd = c->conn->sockfd;
        e->channels[sockfd] = NULL;
    } 
    delChannel(c,NULL);
}

void delEventsLoop(eventsLoop* e){
    shutdown_e(e);
    while (atomic_load(&e->open)!=2);
    delEpollApi(e->api);
    free(e->masks);
    free(e->pending);
    for (int i=0;i<e->capacity;++i){
        Channel* c = e->channels[i];
        if (c==NULL) continue;
        _freeChannel(c,false);
    }
    free(e->channels);
    delWheel(e->t_wheel);
    delBlockQueue(e->tasks);
    delIdGenerator(e->ids);
    free(e);
}


void freeChannel(Channel* c,bool on){
    eventsLoop* e = (eventsLoop*)c->data;
    if (inLoop(e)) _freeChannel(c,on);
    else {
        task_e* t = funcTwoArg(_freeChannel,c,(void*)on);
        push_b(e->tasks,t);
    }
}

void _reg(eventsLoop* e,Channel* c,int event){
    c->data = e;
    c->id = next_i(e->ids);
    int sockfd = c->conn->sockfd;
    if (sockfd>=(e->capacity)){
        int cap = e->capacity;
        Channel** ncs = (Channel**)malloc(2*cap*sizeof(Channel*));
        for (int i=0;i<2*cap;++i) ncs[i] = NULL;
        uchar* nm = (uchar*)malloc(2*cap*sizeof(uchar));
        memcpy(ncs,e->channels,cap);
        memcpy(nm,e->masks,cap);
        free(e->channels);
        free(e->masks);
        e->channels = ncs;
        e->masks = nm;
        e->capacity = 2*cap;
    }
    if (serverSide(e) && e->channels[sockfd]!=NULL && !isChannelOpen(e->channels[sockfd])){
        _freeChannel(e->channels[sockfd],false);
    }
    e->channels[sockfd] = c;
    if (event>0) addEvent(e->api,sockfd,event,true);
    atomic_fetch_add(&e->size,1);
}

void reg(eventsLoop* e,Channel* c,int event){
    if (inLoop(e)) _reg(e,c,event);
    else{
        task_e* t = funcThreeArg(_reg,e,c,event);
        push_b(e->tasks,t);
    }
}

void unreg(Channel* c,bool again){
    eventsLoop* e = (eventsLoop*)c->data;
    if (inLoop(e)) _unreg(c,again);
    else{
        task_e* t = funcTwoArg(_unreg,c,(void*)again);
        push_b(e->tasks,t);
    }
}

void addMask(eventsLoop* e,int sockfd,int mask){
    char* m = e->masks;
    m[sockfd] |= mask;
}

int doPoll(eventsLoop* e,time_out* timeout){
    if (atomic_load(&e->size)==0) return 0;
    epoll_api* api = e->api;
    int dl = timeout->tv_usec+timeout->tv_sec*1000;
    int maxfd = atomic_load(&api->maxfd);
    int fds = (maxfd>0)?maxfd:1;
    int num = epoll_wait(api->epfd,api->ready,fds,dl);
    if (num>0){
        for (int i=0;i<num;++i){
            ep_event* events = &api->ready[i];
            int fdn = events->data.fd;
            e->pending[i] = fdn;
            if (events->events & EPOLLIN) addMask(e,fdn,(RMASK));
            if (events->events & EPOLLOUT) addMask(e,fdn,(WMASK));
        }
    }
    return num;
}

int handleTasks(eventsLoop* e,int num,time_out* deadline){
    time_out now;
    int i = 0;
    for (;i<num;++i){
        bool block = false;
        if (atomic_load(&e->size)==0) block = true;
        task_e* t = (task_e*)poll_b(e->tasks,block);
        if (t==NULL) break;
        invokeArgs_e(t);
        free_taske(t);
        if (!isOpen_e(e)) break;
        if (i%32==0){
            gettimeofday(&now,NULL);
            if (now.tv_sec>deadline->tv_sec || now.tv_usec>deadline->tv_usec)
                break;
        }
    }
    return i;
}

void simpleLoop(eventsLoop* loop){   
    while (!isOpen_e(loop));
    time_out clock;
    gettimeofday(&clock,NULL);
    for (;;){
        if (!isOpen_e(loop)) break;
        int handled = handleTasks(loop,64,&loop->t_wheel->_free);
        updateFreeTime(loop->t_wheel);
        int ready = doPoll(loop,&loop->t_wheel->_free);
        execute_t(loop->t_wheel,&clock);
        if (ready>0){
            for (int i=0;i<ready;++i){
                int fd = loop->pending[i];
                char mask = loop->masks[fd];
                Channel* channel = loop->channels[fd];
                if (mask & (RMASK)){
                    channel->conn->read(channel);
                    loop->masks[fd] &= ~(RMASK);
                }
                if (isChannelOpen(channel) && (mask&(WMASK))){
                    channel->conn->write(channel);
                    loop->masks[fd] &= ~(WMASK);
                }
                loop->pending[i] = 0;
            }
        }
    }
    // need to inform another thread
    atomic_store(&loop->open,2);
}

void writeOp(Channel* channel){
    eventsLoop* e = (eventsLoop*)channel->data;
    int sockfd = channel->conn->sockfd;
    char* writebuf = channel->writeBuf->buf;
    int writePos = channel->writeBuf->pos;
    if (writePos>0){
        int num=0,sign = 0;
        num = writeSocket(sockfd,writebuf,writePos,&sign);
        if (sign==-1){
            int length = writePos-num;
            memcpy(writebuf,writebuf+num,length);
            channel->writeBuf->pos = length;
        }else{
            delEvent(e->api,sockfd,(WEVENT));
            channel->writeBuf->pos = 0;
        } 
    }
}

/**
 * | magic(2B) | id(8B) | type(2B) | length(2B) | content | \r\n | 
*/
void readOp(Channel* channel){
    int sockfd = channel->conn->sockfd;
    int times = 0; // read mutiple times
    char* readbuf; 
    int rbufsize = channel->readBuf->size; // start from 1 !!!
    do{
        int num=0,sign=0;
        readbuf = channel->readBuf->buf;
        int pos = channel->readBuf->pos;
        char exbuf[65536];
        if (isChannelOpen(channel)){
            struct msghdr msg;
            struct iovec vecs[2];
            vecs[0].iov_base = readbuf+pos;
            vecs[0].iov_len = rbufsize-pos;
            vecs[1].iov_base = exbuf;
            vecs[1].iov_len = 65536;
            msg.msg_iov = vecs;
            msg.msg_iovlen = 2;
            num = readSocket(sockfd,&msg,rbufsize-pos+65536,&sign);
            if (sign==-1) break;
            else if (sign==2){
                unreg(channel,true);
                break;
            }
            time(&channel->last_read);
        }
        int capacity = pos+num;
        int channel_hold = (capacity>rbufsize)?rbufsize:capacity;
        if (channel_hold<=0) continue;
        int channel_pos = 0;
        int left_buf = capacity-rbufsize;
        int exbuf_pos = 0;
        Codec c;
        for (;;){
            int d = firstDelimiter(readbuf,channel_hold-channel_pos);
            if (d==-1){
                // if there is no delimiter,find the last magic number
                int m = firstMagic(readbuf,channel_hold-1,channel_hold-channel_pos);
                readbuf = channel->readBuf->buf;
                int cpnum = 0;
                if (m==-1){
                    channel->readBuf->pos = 0;
                    if (left_buf-exbuf_pos>0){
                        cpnum = ((left_buf-exbuf_pos)>rbufsize)?rbufsize:left_buf-exbuf_pos;
                        memcpy(readbuf,exbuf+exbuf_pos,cpnum);
                    }
                }
                else{
                    if (m!=0) memcpy(readbuf,readbuf+m,channel_hold-m);
                    channel->readBuf->pos = channel_hold-m;
                    if (left_buf-exbuf_pos>0){
                        int posnow = channel->readBuf->pos;
                        if (channel_hold==rbufsize){
                            int ed = firstDelimiter(exbuf+exbuf_pos,left_buf-exbuf_pos);
                            if (ed!=-1){
                                cpnum = ed+4;
                                char* mem = (char*)malloc((rbufsize+cpnum)*sizeof(char));
                                memcpy(mem,readbuf,rbufsize);
                                memcpy(mem+rbufsize,exbuf+exbuf_pos,cpnum);
                                free(channel->readBuf->buf);
                                rbufsize += cpnum;
                                channel_hold = rbufsize;
                                channel->readBuf->buf = mem;
                                channel->readBuf->size = rbufsize;
                                channel->readBuf->pos = rbufsize;
                                readbuf = channel->readBuf->buf; 
                            }
                        }
                        else{
                            cpnum = (left_buf-exbuf_pos>rbufsize-posnow)?rbufsize-posnow:left_buf-exbuf_pos;
                            memcpy(channel->readBuf->buf+posnow,exbuf+exbuf_pos,cpnum);
                        } 
                    }
                }
                if (left_buf-exbuf_pos>0){
                    channel_pos = 0;
                    exbuf_pos += cpnum;
                    continue;
                }
                break;
            }
            fill(&c,readbuf,d);
            sign = 0;
            uint16_t magic = readRawInt16(&c,&sign);
            if (sign!=-1 && magic==(MAGIC)){
                eventsLoop* e = (eventsLoop*)channel->data;
                workerGroup* group = (workerGroup*)(e->data);
                uint64_t id;
                uint16_t type;
                uint16_t length;
                uchar* content;
                id = readInt64(&c,&sign);
                if (sign!=-1){
                    type = readInt16(&c,&sign);
                    if (sign!=-1){
                        length = readInt16(&c,&sign);
                        content = (uchar*)MEMALLOC(length);
                        if (length>0 && sign!=-1) readBytes(&c,length,content,&sign);
                    } 
                }
                if (sign!=-1){
                    pros* _pros = group->_pros;
                    if (type<_pros->num){
                        pro* p = _pros->ps[type];
                        if (p!=NULL){
                            addTask_t(p->pool,funcFourArg(p->onEvent,channel,id,length,content));
                        }
                    }
                }
            }
            channel_pos += d+4;
            if (channel_hold-channel_pos<=0 && left_buf-exbuf_pos<0){
                channel->readBuf->pos = 0;
                break;
            }
            readbuf += d+4;
        }
    }while (isChannelOpen(channel) && isOpen_e((eventsLoop*)channel->data) && times++<10);
    return;
}

Channel* findLoopAndReg(workerGroup* group,int fd,int event,addr_ipv4* addr){
    uint32_t tmp = group->index % group->num;
    Thread* t = group->threads[tmp];
    eventsLoop* e = (eventsLoop*)t->data;
    Connection* conn = newConnection(fd,readOp,writeOp);
    Channel* c = newChannel(conn,e);
    c->addr = *addr;
    retainChannel(c);
    reg(e,c,event);
    ++(group->index);
    return c;
}

void listenLoop(pair* p){
    workerGroup* group = (workerGroup*)p->a;
    server* s = (server*)p->b;
    Option* o = newOption();
    o->nodelay = true;
    o->isTCP = true;
    addr_ipv4* addr = (addr_ipv4*)malloc(sizeof(addr_ipv4));
    int addr_size = sizeof(addr_ipv4);
    while (isOpen_s(s)){
        memset(addr,0,sizeof(addr_ipv4));
        int fd = acceptConnection(s->sockfd,addr,&addr_size,o);
        findLoopAndReg(group,fd,(REVENT),addr);
    }
    free(addr);
    delOption(o);
}

workerGroup* initGroup(int num,pros* _pros,bool isServer,server* s){
    workerGroup* group = (workerGroup*)malloc(sizeof(workerGroup));
    if (num<=0) num = sysconf(_SC_NPROCESSORS_ONLN)*2;
    group->num = num;
    group->threads = (Thread**)malloc(num*sizeof(Thread*));
    group->_pros = _pros;
    group->index = 0;
    for (int i=0;i<num;++i){
        eventsLoop* loop = newEventsLoop(i+1);
        loop->data = group;
        Thread* t = newThread(simpleLoop,loop,loop,delEventsLoop);
        loop->thread = t;
        group->threads[i] = t; 
        startLoop_e(loop);
    }
    if (isServer){
        pair* p = newPair(group,s);
        Thread* boss = newThread(listenLoop,p,s,delServer);
        group->listener = boss;
    }
    return group;

}

void delGroup(workerGroup* group){
    for (int i=0;i<group->num;++i){
        Thread* t = group->threads[i];
        delThread(t,delEventsLoop);
    }
    free(group->threads);
    if (group->listener!=NULL){
        delThread(group->listener,delServer);
    }
    pros* _pros = group->_pros;
    if (_pros!=NULL){
        for (int i=0;i<_pros->num;++i){
            if (_pros->ps[i]==NULL) continue;
            delPro(_pros->ps[i]);
        }
        free(_pros);
    }
    free(group);
}

Channel* connTo_e(workerGroup* group,addr_ipv4* addr){
    Option* o = newOption();
    o->fastopen = true;
    o->isTCP = true;
    o->keepalive = true;
    o->nodelay = true;
    int fd = newTcpSocket(o);
    if (connectTo(fd,addr)<0) return NULL;
    fcntl(fd, F_SETFL, fcntl(fd,F_GETFL,0) | O_NONBLOCK);
    return findLoopAndReg(group,fd,(REVENT),addr);
}

void writeToBuf1(Channel* c,uint32_t type,const uchar* bytes,uint16_t length){
    if (!isChannelOpen(c)) return;
    eventsLoop* e = (eventsLoop*)c->data;
    writeToBuf(c,next_i(e->ids),type,bytes,length);
    // have to make sure bytes is from mempool
    DEMEMALLOC((void*)bytes);
    addEvent(e->api,c->conn->sockfd,(WEVENT),false);
}

void writeChannel(Channel* c,uint32_t type,const uchar* bytes,uint16_t length){
    eventsLoop* e = (eventsLoop*)c->data;
    uchar* pchar = (uchar*)MEMALLOC((void*)length);
    memcpy(pchar,bytes,length);
    if (inLoop(e)) writeToBuf1(c,type,pchar,length);
    else{
        task_e* t = funcFourArg(writeToBuf1,c,type,pchar,length);
        push_b(e->tasks,t);
    }
}

void scheduleRoutine(eventsLoop* e,task_e* task,long seconds,bool again){
    if (inLoop(e)) addNewEvent_w(e->t_wheel,task,seconds,again);
    else{
        task_e* te = funcFourArg(addNewEvent_w,e->t_wheel,task,seconds,(void*)again);
        push_b(e->tasks,te);
    }
}

void addRoutine(workerGroup* g,task_e* task,long seconds,bool again){
    for (int i=0;i<g->num;++i){
        Thread* t = g->threads[i];
        eventsLoop* e = (eventsLoop*)t->data;
        scheduleRoutine(e,task,seconds,again);
    }
}

void syncOn(workerGroup* g){
    sync();
    delGroup(g);
}
