#include "TimeWheel.h"

void empty_t(time_out* t){
    t->tv_sec = 0;
    t->tv_usec = 0;
}

bool compare_t(time_out* a,time_out* b){
    if (a->tv_sec>b->tv_sec) return true;
    else if (a->tv_sec<b->tv_sec) return false;
    return a->tv_usec>b->tv_usec;
}

tnode* newTNode(task_e* task,long seconds,bool again){
    tnode* n = (tnode*)malloc(sizeof(tnode));
    n->again = again;
    n->_time.tv_sec = seconds;
    n->_time.tv_usec = 0;
    n->task = task;
    return n;
}

void delTNode(tnode* t){
    free_taske(t->task);
    free(t);
}

chain* newChain(wheel* w,long s){
    chain* c = (chain*)malloc(sizeof(chain));
    c->nodes = NULL;
    c->w = w;
    c->empty = true;
    c->t.tv_sec = s;
    c->t.tv_usec = 0;
    return c;
}

void addTNode(wheel* w,tnode* n){
    long s = n->_time.tv_sec;
    if (s<=w->slots){
        if (compare_t(&w->_free,&n->_time)) w->_free = n->_time;
        chain* c = w->chains[s-1];
        if (c->empty && c->nodes==NULL){
            c->nodes = newNode(n);
            c->empty = false;
            return;
        }
        list_node* head = c->nodes;
        addNode(head,n);
    }
}

void delChain(chain* c){
    delList(c->nodes,NULL);
    free(c);
}

wheel* newWheel(int slots){
    wheel* w = (wheel*)malloc(sizeof(wheel));
    w->slots = slots;
    w->chains = (chain**)malloc(slots*sizeof(chain*));
    for (int i=1;i<=slots;++i){
        w->chains[i-1] = newChain(w,i);
    }
    empty_t(&w->_free);
    return w;
}

void delWheel(wheel* w){
    for (int i=0;i<w->slots;++i){
        delChain(w->chains[i]);
    }
    free(w);
}

long timePass(time_out* clock){
    time_out t;
    gettimeofday(&t,NULL);
    long pass = t.tv_sec - clock->tv_sec;
    clock->tv_sec = t.tv_sec;
    clock->tv_usec = t.tv_usec;
    return pass;
}

void execute_t(wheel* w,time_out* clock){
    long s = timePass(clock);
    list_node* nHead = NULL;
    for (int i=0;i<w->slots;++i){
        chain* c = w->chains[i];
        if (!c->empty){
            list_node* head = c->nodes;
            while (head!=NULL){
                tnode* n = (tnode*)head->arg;
                if (i<s){
                    invokeArgs_e(n->task);
                    if (n->again){
                        if (nHead==NULL) nHead = newNode(n);
                        else addNode(nHead,n);
                    }
                    else delTNode(n);
                }
                else n->_time.tv_sec -= s;
                head = head->next;
            }
            if (i<s){
                c->nodes = NULL;
                if (nHead!=NULL) c->nodes = nHead;
                else c->empty = true;
            }
            else if (s!=0){
                long tpos = c->t.tv_sec-s;
                if (w->chains[tpos-1]->nodes==NULL)
                    w->chains[tpos-1]->nodes = c->nodes;
                else linkNode(w->chains[tpos-1]->nodes,c->nodes);
                if (w->chains[tpos-1]->empty){
                    w->chains[tpos-1]->empty = false;
                } 
                c->nodes = NULL;
                c->empty = true;
            }
        }
    }
}

void updateFreeTime(wheel* w){
    empty_t(&w->_free);
    for (int i=1;i<=w->slots;++i){
        if (!w->chains[i-1]->empty){
            w->_free.tv_sec = i;
            break;
        }
    }
}

void addNewEvent_w(wheel* w,task_e* task,long seconds,bool again){
    tnode* n = newTNode(task,seconds,again);
    addTNode(w,n);
}