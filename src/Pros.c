#include "Pros.h"

pros* newPros(int num){
    pros* p = (pros*)malloc(sizeof(pros));
    p->num = num;
    p->ps = (pro**)malloc(num*sizeof(pro*));
    for (int i=0;i<num;++i){
        p->ps[i] = NULL;
    }
    return p;
}

bool addPro(pros* _ps,pro* p){
    if (p->type>=_ps->num) return false;
    _ps->ps[p->type] = p;
    return true;
}

pro* newPro(int type,void(*onEvent)(Channel*,uint64_t,uint16_t,uchar*),int threadNum){
    pro* p = (pro*)malloc(sizeof(pro));
    p->type = type;
    p->onEvent = onEvent;
    p->pool = newThreadPool(threadNum);
    return p;
}

void delPro(pro* p){
    delThreadPool(p->pool);
    free(p);
}

