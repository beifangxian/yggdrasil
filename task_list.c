#include "task_list.h"
#include "mem_pool.h"

extern mp_pool_t mp_task;

/*头节点的创建*/
tasklist *list_init()
{
    // tasklist *head = malloc(sizeof(tasklist));
    tasklist *head = mp_malloc(&mp_task);
    if(head == NULL)
    {
        fprintf(stderr, "mp_malloc: failed to allocate memory for task head node\n");
        return NULL;
    }
    head->taskp = NULL;
    head->taskarg = NULL;
    head->next = NULL;
    return head;
}

/*
    节点的添加要确保在某些情况突发时，锁能够安全的保护共享资源
    以及突发情况退出时能够顺利的把锁解开
*/

/*新建节点，并且尾插*/
int list_add_tail(void *(* newtask)(void *),void *newtaskarg, thread_pool *pool)
{
    pthread_mutex_lock(&pool->mutex);
    /*背压:队列满时生产者阻塞,等待消费者取走任务后唤醒*/
    while(pool->task_num >= pool->max_task_num)
    {
        pthread_cond_wait(&pool->cond_not_full, &pool->mutex);
    }
    tasklist *newnode = mp_malloc(&mp_task);
    if(newnode == NULL)
    {
        pthread_mutex_unlock(&pool->mutex);
        fprintf(stderr, "Memory allocation failed for new task node\n");
        return -1;
    }
    tasklist *p = pool->task_head;//如果在锁外面进行则会有概率被其他线程修改
    while(p->next != NULL)
    {
        p = p->next;
    }
    newnode->taskp = newtask;
    newnode->taskarg = newtaskarg;
    newnode->next = NULL;
    p->next = newnode;
    pool->task_num++;
    pthread_mutex_unlock(&pool->mutex);
    /*唤醒一个消费者线程*/
    pthread_cond_signal(&pool->cond);
    return 0;
}

/*新建节点，并且头插*/
int list_add_head(void *(*newtask)(void *), void *newtaskarg, thread_pool *pool)
{
    pthread_mutex_lock(&pool->mutex);
    /*背压:队列满时生产者阻塞,等待消费者取走任务后唤醒*/
    while(pool->task_num >= pool->max_task_num)
    {
        pthread_cond_wait(&pool->cond_not_full, &pool->mutex);
    }
    tasklist *newnode = mp_malloc(&mp_task);
    if(newnode == NULL)
    {
        pthread_mutex_unlock(&pool->mutex);
        fprintf(stderr, "Memory allocation failed for new task node\n");
        return -1;
    }
    newnode->taskp = newtask;
    newnode->taskarg = newtaskarg;
    newnode->next = pool->task_head->next;
    pool->task_head->next = newnode;
    pool->task_num++;
    pthread_mutex_unlock(&pool->mutex);
    pthread_cond_signal(&pool->cond);
    return 0;
}

/*调用任务*/
int list_show(tasklist * head)
{
    tasklist *p = head;
    while(p->next != NULL)
    {
        p = p->next;

        /*执行节点里面存放的任务*/
        (p->taskp)(p->taskarg);
    }
    return 0;
}