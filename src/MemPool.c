#include "MemPool.h"

static mem_pool* mp_glo;
static volatile bool inited = false;

void del_mp_global(void){
    delMemPool(mp_glo);
}

void init_mp_global(uint16_t max_size){
    mp_glo = newMemPool(max_size);
    inited = true;
    atexit(del_mp_global);
}

void* alloc_mp_glo(uint16_t size){
    if (!inited){
        init_mp_global(8192);
        inited = true;
    } 
    return alloc_mp(mp_glo,size);
}

bool dealloc_mp_glo(void* mem){
    return dealloc_mp(mp_glo,mem);
}

uint16_t findIndex(uint16_t size){
    return (size+((ALIGN)-1))/(ALIGN);
}

uint16_t roundup(uint16_t size){
    return (size+((ALIGN)-1)&~((ALIGN)-1));
}

uint16_t maxBlocks(uint16_t size){
    if (size>=(SIZEUNIT)) return (MAXBLOCKNUM);
    else return (MAXBLOCKNUM)*((SIZEUNIT)/size);
}

mem_pool* newMemPool(uint16_t max_size){
    uint16_t ms = roundup(max_size);
    uint16_t tablelen = findIndex(ms);
    mem_pool* mp = (mem_pool*)malloc(sizeof(mem_pool)+tablelen*sizeof(part));
    mp->max_size = ms;
    mp->parts = (part*)((void*)mp+sizeof(mem_pool));
    for (int i=0;i<tablelen;++i){
        mp->parts[i].idle = 0;
        mp->parts[i].head = NULL;
        pthread_spin_init(&mp->parts[i].mtx,0);
    }
    mp->big_block = NULL;
    pthread_spin_init(&mp->mtx,0);
    return mp;
}

void delMemPool(mem_pool* mp){
    uint16_t len = findIndex(mp->max_size);
    for (int i=0;i<len;++i){
        if (mp->parts[i].idle>0){
            node_m* nm;
            while (mp->parts[i].head!=NULL){
                nm = mp->parts[i].head->next;
                free((void*)mp->parts[i].head);
                mp->parts[i].head = nm;
            }
        }
    }
    part_b* pb;
    while (mp->big_block!=NULL){
        pb = mp->big_block->next;
        free(mp->big_block);
        mp->big_block = pb;
    }
    free(mp);
}

void* alloc_mp(mem_pool* mp,uint16_t size){
    if (size>(MAXSIZE)) return NULL;
    uint16_t rsize = roundup(size+sizeof(uint16_t)+sizeof(uint8_t));
    if (rsize>=mp->max_size){
        void* area = malloc(sizeof(part_b)+rsize);
        part_b* pb = (part_b*)area;
        pb->next = NULL;
        pb->prev = NULL;
        pthread_spin_lock(&mp->mtx);
        if (mp->big_block==NULL) mp->big_block = pb;
        else{
            pb->prev = mp->big_block;
            mp->big_block->next = pb;
        } 
        pthread_spin_unlock(&mp->mtx);
        *(uint16_t*)(area+sizeof(part_b)) = rsize;
        *(uint8_t*)(area+sizeof(part_b)+sizeof(uint16_t)) = 1;
        return area+sizeof(part_b)+sizeof(uint16_t)+sizeof(uint8_t);
    }
    uint16_t index = findIndex(rsize);
    pthread_spin_lock(&mp->parts[index].mtx);
    node_m* h = mp->parts[index].head;
    if (h!=NULL){
        void* ptr = (void*)h;
        mp->parts[index].head = h->next;
        *(uint16_t*)ptr = rsize;
        *(uint8_t*)(ptr+sizeof(uint16_t)) = 1;
        mp->parts[index].idle--;
        pthread_spin_unlock(&mp->parts[index].mtx);
        return ptr+sizeof(uint16_t)+sizeof(uint8_t); 
    }
    pthread_spin_unlock(&mp->parts[index].mtx);
    void* area = malloc((rsize>sizeof(node_m))?rsize:sizeof(node_m));
    *(uint16_t*)area = rsize;
    *(uint8_t*)(area+sizeof(uint16_t)) = 1;
    return area+sizeof(uint16_t)+sizeof(uint8_t);
}

bool dealloc_mp(mem_pool* mp,void* mem){
    uint8_t ref = *(uint8_t*)(mem-sizeof(uint8_t));
    if (ref>1){
        *(uint8_t*)(mem-sizeof(uint8_t)) = ref-1;
        return false;
    }
    uint16_t size = *(uint16_t*)(mem-sizeof(uint8_t)-sizeof(uint16_t));
    if (size>=mp->max_size){
        part_b* pb = (part_b*)(mem-sizeof(uint8_t)-sizeof(uint16_t)-sizeof(part_b));
        pthread_spin_lock(&mp->mtx);
        if (pb==mp->big_block) mp->big_block = pb->next;
        else{
            part_b* pr = pb->prev;
            pr->next = pb->next;
        }
        pthread_spin_unlock(&mp->mtx);
        free((void*)pb);
        return true;
    }
    uint16_t index = findIndex(size);
    void* ptr = mem-sizeof(uint8_t)-sizeof(uint16_t);
    pthread_spin_lock(&mp->parts[index].mtx);
    if (mp->parts[index].idle>maxBlocks(size)) free(ptr);
    else{
        node_m* nm = (node_m*)ptr;
        nm->next = NULL;
        if (mp->parts[index].head==NULL) 
            mp->parts[index].head = nm;
        else{
            nm->next = mp->parts[index].head;
            mp->parts[index].head = nm;
        }
        mp->parts[index].idle++;
    }
    pthread_spin_unlock(&mp->parts[index].mtx);
    return true;
}

void retain_mem(void* mem){
    uint8_t ref = *(uint8_t*)(mem-sizeof(uint8_t));
    if (ref==0xff) return;
    *(uint8_t*)(mem-sizeof(uint8_t)) = ref+1;
}