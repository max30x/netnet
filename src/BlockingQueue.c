#include "BlockingQueue.h"

Node* newNode_b(void* val){
    Node* n = (Node*)malloc(sizeof(Node));
    n->val = val;
    n->next = NULL;
    return n;
}

void delNode_b(Node* n){
    free(n);
}

bool isOk(block_queue* q){
    return atomic_load(&q->wake_up)==0;
}

block_queue* newBlockQueue(int bufnum){
    block_queue* q = (block_queue*)malloc(sizeof(block_queue));
    q->bufnum = bufnum;
    q->bufs = (Node**)malloc(bufnum*sizeof(Node*));
    q->locks = (pthread_mutex_t**)malloc(bufnum*sizeof(pthread_mutex_t*));
    q->conds = (pthread_cond_t**)malloc(bufnum*sizeof(pthread_cond_t*));
    for (int i=0;i<bufnum;++i){
        q->bufs[i] = NULL;
        q->locks[i] = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(q->locks[i],NULL);
        q->conds[i] = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
        pthread_cond_init(q->conds[i],NULL);
    }
    atomic_init(&q->write_pos,0);
    atomic_init(&q->read_pos,0);
    atomic_init(&q->wake_up,0);
    atomic_init(&q->ref,0);
    return q;
}

void _delBlockQueue(block_queue* q){
    for (int i=0;i<q->bufnum;++i){
        Node *n = q->bufs[i];
        Node *next = NULL;
        while (n!=NULL){
            next = n->next;
            if(n!=NULL) delNode_b(n);
            n = next;
        }
        pthread_mutex_destroy(q->locks[i]);
        free(q->locks[i]);
        pthread_cond_destroy(q->conds[i]);
        free(q->conds[i]);
    }
    free(q->locks);
    free(q->conds);
    free(q->bufs);
    free(q);
}

void delBlockQueue(block_queue* q){
    wake_all(q);
    while (atomic_load(&q->ref)!=0)
    _delBlockQueue(q);
}

void addBack(block_queue* q,int index,void* val){
    Node* node = q->bufs[index];
    if (node == NULL){
        q->bufs[index] = newNode_b(val);
        return;
    }
    while (node->next!=NULL) node = node->next;
    node->next = newNode_b(val);
}

Node* popBack(block_queue* q,int index){
    Node* node = q->bufs[index];
    Node* prev = NULL;
    while (node->next!=NULL){
        prev = node;
        node = node->next;
    }
    if (prev!=NULL) prev->next = NULL;
    else q->bufs[index] = NULL;
    return node;
}

Node* popFirst(block_queue* q,int index){
    Node* node = q->bufs[index];
    if (node!=NULL){
        q->bufs[index] = node->next;
    }
    return node;
}

int getAndInc(atomic_int* num){
    int _n = atomic_load(num);
    while (!atomic_compare_exchange_weak(num,&_n,_n+1));
    return _n;
}

void subDown(atomic_int* num){
    int _n = atomic_load(num);
    while (!atomic_compare_exchange_weak(num,&_n,_n-1));
}

void push_b(block_queue* q,void* val){
    if (!isOk(q)) return;
    atomic_fetch_add(&q->ref,1);
    int wIndex = getAndInc(&q->write_pos);
    int pos = wIndex % q->bufnum;
    pthread_mutex_t* mtx = q->locks[pos];
    pthread_mutex_lock(mtx);
    addBack(q,pos,val);
    int rIndex = atomic_load(&q->read_pos);
    if (wIndex<=rIndex-1){
        pthread_cond_t* cond = q->conds[pos];
        pthread_cond_signal(cond);
    }
    pthread_mutex_unlock(mtx);
    atomic_fetch_sub(&q->ref,1);
}

void* poll_b(block_queue* q,bool block){
    if (!isOk(q)) return NULL;
    atomic_fetch_add(&q->ref,1);
    int rIndex = getAndInc(&q->read_pos);
    int pos = rIndex % q->bufnum;
    pthread_mutex_t* mtx = q->locks[pos];
    pthread_mutex_lock(mtx);
    while (isOk(q) && q->bufs[pos]==NULL){
        if (!block){
            pthread_mutex_unlock(mtx);
            subDown(&q->read_pos);
            atomic_fetch_sub(&q->ref,1);
            return NULL;
        }
        int sign = (q->bufs[pos]==NULL)?0:1;
        pthread_cond_t* cond = q->conds[pos];  
        pthread_cond_wait(cond,mtx);
    }
    if (!isOk(q)){
        pthread_mutex_unlock(mtx);
        subDown(&q->read_pos);
        atomic_fetch_sub(&q->ref,1);
        return NULL;
    }
    Node* _node = popFirst(q,pos);
    void* ptr = _node->val;
    delNode_b(_node);
    pthread_mutex_unlock(mtx);
    atomic_fetch_sub(&q->ref,1);
    return ptr;
}

void wake_all(block_queue* q){
    atomic_store(&q->wake_up,1);
    for (int i=0;i<q->bufnum;++i){
        pthread_cond_t* cond = q->conds[i];
        pthread_mutex_t* mtx = q->locks[i];
        pthread_mutex_lock(mtx);
        pthread_cond_broadcast(cond);
        pthread_mutex_unlock(mtx);
    }
}

bool isEmpty_b(block_queue* q){
    int rIndex = atomic_load(&q->read_pos);
    int wIndex = atomic_load(&q->write_pos);
    return wIndex<=rIndex;
}