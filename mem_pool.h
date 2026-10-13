#ifndef _MEM_POOL_H
#define _MEM_POOL_H

#include "mytypes.h"

/*内存池的建立*/
int mp_init(size_t bs, size_t bc, void *m, mp_pool_t *mp);

/*分配内存*/
void *mp_malloc(mp_pool_t *mp);

/*释放已分配的内存*/
void mp_free(mp_pool_t *mp, void *b);

#endif