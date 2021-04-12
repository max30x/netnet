#ifndef MEMBLOCKPOOL_GLO
#define MEMBLOCKPOOL_GLO

#include "Include.h"

#define ALIGN 8
#define SIZEUNIT 2048
#define MAXBLOCKNUM 16
#define MAXSIZE 65536

#define MEMALLOC alloc_mp_glo
#define DEMEMALLOC dealloc_mp_glo

typedef struct node_m{
    struct node_m* next;
}node_m;

typedef struct part{
    uint16_t idle;
    node_m* head;
    pthread_spinlock_t mtx;
}part;

typedef struct part_b{
    struct part_b *prev,*next;
}part_b;

typedef struct mem_pool{
    uint32_t max_size;
    part_b* big_block;
    part* parts;
    pthread_spinlock_t mtx;
}mem_pool;

mem_pool* newMemPool(uint16_t max_size);
void delMemPool(mem_pool* mp);

void* alloc_mp(mem_pool* mp,uint16_t size);
bool dealloc_mp(mem_pool* mp,void* mem);
void retain_mem(void* mem);

bool dealloc_mp_glo(void* mem);
void* alloc_mp_glo(uint16_t size);

#endif