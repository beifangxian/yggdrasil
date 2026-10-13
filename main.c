#include "thread_pool.h"
#include "task_list.h"
#include "mem_pool.h"
#include "tree.h"


/*任务内存池*/
unsigned char array_task[TASK_SIZE * MAX_TASK];
mp_pool_t mp_task;
/*树内存池*/
unsigned char array_tree[TREE_SIZE * MAX_TREE];
mp_pool_t mp_tree;

unsigned short file_count;
unsigned short dirent_count;

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("如果要查看目录结构请输入要查看的要查看的目录的路径\n");
		printf("如果要复制文件请输入源目录或文件路径	和	要复制到的目标目录路径\n");
		return 0;
	}
	file_count = 0;
	dirent_count = 0;
	/*分级内存池初始化*/
    mp_init(TASK_SIZE,  MAX_TASK, array_task, &mp_task);
    mp_init(TREE_SIZE,  MAX_TREE, array_tree, &mp_tree);

	/*任务链表与线程池初始化*/
    tasklist *head = list_init();
    thread_pool *pool = pool_init(5, head);

	/*树初始化*/
	tree *t = tree_init(argv[1]);
	tree_build((void *)t);
	if(argc == 2)
	{
		show_tree(t, t->layer);
		printf("%hd:files\t%hd:directories\n",file_count,dirent_count);
	}
	/*多线程拷贝目录树到指定目录*/
	if(argc == 3)
	{
		tree_copy(t,argv[2],pool);
	}
	
    /*销毁线程池:会等所有任务执行完再返回(处理完剩余任务再退)*/
    pool_destroy(pool, head);

	/*必须在 pool_destroy 之后打印,此时所有任务已执行完,统计才准确*/
	if(argc == 3)
	{
		printf("拷贝完成：成功 %u 个，失败 %u 个\n", copy_success_count, copy_fail_count);
	}
    return 0;
}