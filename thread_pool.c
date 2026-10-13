#include "thread_pool.h"
#include "mem_pool.h"

extern mp_pool_t mp_task;

/*线程池共用的任务函数*/
void *routine(void *arg)
{
    thread_pool *pool = (thread_pool *)arg;
    tasklist *taskp = NULL;
    while(1)
    {
        pthread_mutex_lock(&pool->mutex);//锁上后执行任务
        /*检查线程池是否关闭*/
        while(pool->task_head->next == NULL && pool->pool_switch == true)//如果链表为空，那么循环判断是否有任务建立
        {
            pthread_cond_wait(&pool->cond,  &pool->mutex);//阻塞线程
        }
        /*必须写下面*/
        if(pool->pool_switch == false && pool->task_num <= 0)
        {
            pthread_mutex_unlock(&pool->mutex);
            pthread_exit(NULL);
        }
        taskp = pool->task_head->next;//拿到任务
        /*在锁内完成共享资源的修改，以防出现多个线程同时访问并修改同一资源的错误情况*/
        pool->task_num--;
        pool->task_head->next = taskp->next;
        /*取走一个任务后，通知被阻塞的生产者队列有空位了(背压)*/
        pthread_cond_broadcast(&pool->cond_not_full);
        pthread_mutex_unlock(&pool->mutex);
        (taskp->taskp)(taskp->taskarg);
        mp_free(&mp_task, taskp);
    }
}

/*线程池的创建*/
thread_pool *pool_init(int n, tasklist *head)
{
    thread_pool *pool = malloc(sizeof(thread_pool));
    if(pool == NULL)
    {
        perror("线程池创建失败\n");
        return NULL;
    }
    pool->thread_num = n;
    pool->thread_id = malloc(n * sizeof(pthread_t));
    /*设置线程分离属性*/
    // pthread_attr_init(&pool->thread_attr);
    // pthread_attr_setdetachstate(&pool->thread_attr, PTHREAD_CREATE_DETACHED);
    if(pool == NULL)
    {
        perror("线程ID创建失败\n");
        return NULL;
    }
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);
    pthread_cond_init(&pool->cond_not_full, NULL);
    pool->task_head = head;
    pool->pool_switch = true;
    /*队列上限:头节点占 1 块内存池,任务节点上限为 MAX_TASK-1(背压阈值)*/
    pool->max_task_num = MAX_TASK - 1;
    for(int i = 0; i < n; i++)
    {
        int ret = pthread_create(pool->thread_id + i, NULL, routine, pool);
    }
    return pool;
}

/*线程池的销毁*/
int pool_destroy(thread_pool *pool, tasklist *head)
{
    /*加锁写关闭标志,与工作线程在锁内读保持一致,避免数据竞争*/
    pthread_mutex_lock(&pool->mutex);
    pool->pool_switch = false;
    /*唤醒所有线程，不唤醒线程无法回收*/
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    for (int i = 0; i < pool->thread_num; i++)
    {
        pthread_join(*(pool->thread_id + i), NULL);
    }
    free(pool->thread_id);
    free(pool);
    
    return 0;
}