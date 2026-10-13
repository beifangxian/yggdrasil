#ifndef _TASK_LIST
#define _TASK_LIST

#include "mytypes.h"

tasklist *list_init();
int list_add_tail(void *(* newtask)(void *),void *newtaskarg, thread_pool *pool);
int list_add_head(void *(*newtask)(void *), void *newtaskarg, thread_pool *pool);
int list_show(tasklist * head);

#endif