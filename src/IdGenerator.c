#include "IdGenerator.h"

id_generator* newIdGenerator(uint8_t oid){
    if (oid>(MAXINS)) return NULL;
    id_generator* g = (id_generator*)malloc(sizeof(id_generator));
    time_t tmp;
    time(&tmp);
    g->start_t = tmp;
    g->last_t = -1;
    g->sequence = 0;
    g->object_id = oid;
    return g;
}

void delIdGenerator(id_generator* i){
    free(i);
}

long int next_i(id_generator* i){
    time_t t;
    time(&t);
    if (i->last_t==t){
        if (i->sequence>(MAXSEQ)){
            i->sequence = -1;
            do{
                time(&t);
            }while (t==i->last_t);
        }
        ++(i->sequence);
    }
    else i->sequence = 0;
    i->last_t = t;
    return (t-i->start_t)<<((INS)+(SEQ)) |
            (i->object_id)<<((INS)) |
            (i->sequence);
}