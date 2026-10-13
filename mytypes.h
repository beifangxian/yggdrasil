#ifndef _MYTYPES_H
#define _MYTYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  //open的头文件
#include <unistd.h>  //read的头文件
#include <errno.h>  //perror的头文件
#include <linux/input.h>  //输入子系统模型有关的头文件
#include <sys/ioctl.h>   //ioctl的头文件
#include <sys/mman.h>  //mmap内存映射
#include <dirent.h>   //目录操作
#include <sys/wait.h>  //wait的头文件
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <stddef.h>

#define PRINT_MID printf("├─── ")
#define PRINT_LAST printf("└─── ")

// (2)、宏定义(函数、变量、常量)
#define BGC_BLACK          "\033[40m"  // 背景色(background color)
#define BGC_DARK_RED       "\033[41m"  // 深红		    
#define BGC_GREEN          "\033[42m"  // 绿色
#define BGC_YELLOW         "\033[43m"  // 黄色
#define BGC_BLUE           "\033[44m"  // 蓝色
#define BGC_PURPLE         "\033[45m"  // 紫色
#define BGC_DARK_GREEN     "\033[46m"  // 深绿
#define BGC_WHITE          "\033[47m"  // 白色
									                                        
#define FC_BLACK           "\033[30m"  // 字体色(font color)
#define FC_RED             "\033[31m"  // 红色	
#define FC_GREEN           "\033[32m"  // 绿色
#define FC_YELLOW          "\033[33m"  // 黄色
#define FC_BLUE            "\033[34m"  // 蓝色
#define FC_PURPLE          "\033[35m"  // 紫色
#define FC_DARK_GREEN      "\033[36m"  // 深绿
#define FC_WHITE           "\033[37m"  // 白色

#define SE_CLOSE           "\033[0m"  // 其它特效(Special effects)

#define MAX_FILENAME 512
#define MAX_TASK 1024
#define MAX_TREE 2048
#define TASK_SIZE sizeof(tasklist)
#define TREE_SIZE sizeof(tree)

/*文件拷贝路径存储结构体*/
typedef struct file_path
{
    char srcfile_path[512];
    char goalfile_path[1024];
}file_path;

/*二叉树孩子兄弟存储法*/
typedef struct tree
{
     struct tree *first_child;//第一个子节点
     struct tree *next_sibling;//其余兄弟节点
     struct tree *parent;//父节点
     char filename[MAX_FILENAME];//文件名
     int layer;//距离根节点几层
     bool is_directory;//是否是目录文件
}tree;

/*内存池结构体*/
typedef struct mp_pool
{
    size_t block_size;              // 每个块的大小
    size_t free_count;              // 连续空闲区还剩多少块
    void *free_list_head;           // 碎片空闲块链表头
    void *free_continuous_ptr;      // 连续空闲区当前位置
    pthread_mutex_t mutex;          // 保护内存池并发访问的互斥锁
} mp_pool_t;

/*串联释放后的内存块链表*/
typedef struct block {
    void *next;
}block;

/*任务链表结构体*/
typedef struct tasklist
{
    /*数据域*/
    void *(* taskp)(void *);//存放任务的函数指针
    void *taskarg;//传递给函数指针的参数

    /*指针域*/
    struct tasklist* next;
}tasklist;

/*线程池结构体*/
typedef struct thread_pool
{
    pthread_attr_t thread_attr;//线程属性
    pthread_mutex_t mutex;//互斥锁
    pthread_cond_t cond;//条件变量(消费者等待任务)
    pthread_cond_t cond_not_full;//条件变量(生产者等待队列有空位,用于背压)
    pthread_t *thread_id;
    tasklist* task_head;
    int thread_num;
    int task_num;
    int max_task_num;//队列上限(背压阈值)
    /*
        定义线程池的开关是为了防止在主线程关闭线程池的使用时子线程还在使用的情况
    */
    bool pool_switch;//线程池的开关
}thread_pool;

#endif