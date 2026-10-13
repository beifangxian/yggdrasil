#ifndef _TREE_H
#define _TREE_H

#include "mytypes.h"

tree *tree_init(const char *pathname);

void tree_child(tree *t,tree *new_t);

void *tree_build(void *vt);

void print_vertical(tree *t, int x);

void show_tree(tree *t, int p);

//多线程执行函数
void *copy_file(void *arg);

void tree_copy(tree *t, char *goal, thread_pool *pool);

/*拷贝统计:成功/失败文件数(供 main 汇总打印)*/
extern unsigned int copy_success_count;
extern unsigned int copy_fail_count;

#endif