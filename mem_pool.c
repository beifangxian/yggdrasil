#include "mem_pool.h"

int mp_init(size_t block_size, size_t block_count, void *mem_start, mp_pool_t *mp)
{
    if(mem_start == NULL || mp == NULL)
    {
        printf("初始地址为NULL或者mp为NULL\n");
        return -1;
    }
    if(block_size < sizeof(void *))
    {
        printf("错误：块大小太小，至少需要 %zu 字节（存储链表指针）\n", sizeof(void *));
        return -1;
    }
    
    if(block_count == 0)
    {
        printf("错误：块数量必须大于 0\n");
        return -1;
    }
    
    // 2. 初始化结构体
    mp->block_size = block_size;
    mp->free_count = block_count;           // 连续空闲块数量
    mp->free_list_head = NULL;              // 碎片链表为空
    mp->free_continuous_ptr = mem_start;    // 连续空闲区起始地址
    pthread_mutex_init(&mp->mutex, NULL);   // 初始化内存池互斥锁
    
    return 0;
}

void *mp_malloc(mp_pool_t *mp)
{
    pthread_mutex_lock(&mp->mutex);
    /*优先分配空闲区的内存块*/
    if(mp->free_count > 0)
    {
        mp->free_count--;
        void *b = mp->free_continuous_ptr;
        mp->free_continuous_ptr = (void *)(((unsigned char *)mp->free_continuous_ptr) + mp->block_size);
        pthread_mutex_unlock(&mp->mutex);
        return b;
    }
    /*分配被链表串起来的空闲块的内存块*/
    else if(mp->free_list_head)
    {
        void *b = mp->free_list_head;
        mp->free_list_head = ((block *)mp->free_list_head)->next;
        pthread_mutex_unlock(&mp->mutex);
        return b;
    }
    pthread_mutex_unlock(&mp->mutex);
    return NULL;
}

void mp_free(mp_pool_t *mp, void *p)
{
    pthread_mutex_lock(&mp->mutex);
    ((block *)p)->next = mp->free_list_head;
    mp->free_list_head = p;
    pthread_mutex_unlock(&mp->mutex);
}