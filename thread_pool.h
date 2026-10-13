#ifndef _THREAD_POOL
#define _THREAD_POOL

#include "mytypes.h"

void *routine(void *arg);
thread_pool *pool_init(int n, tasklist *head);
int pool_destroy(thread_pool *pool, tasklist *head);

#endif