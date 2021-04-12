#ifndef IDGENERATOR_GLO
#define IDGENERATOR_GLO

#include "Include.h"

#define INS 7
#define SEQ 15

#define MAXINS -1L^(-1L<<(INS))
#define MAXSEQ -1L^(-1L<<(SEQ))

/**
 * | 0 | timestamp(41bit) | instance(7bit) | sequence(15bit) |
*/
typedef struct id_generator{
    time_t start_t;
    time_t last_t;
    uint8_t object_id;
    uint16_t sequence;
}id_generator;

id_generator* newIdGenerator(uint8_t oid);
void delIdGenerator(id_generator* i);

long int next_i(id_generator* i);

#endif