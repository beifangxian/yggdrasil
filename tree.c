#include "tree.h"
#include "mem_pool.h"
#include "task_list.h"

extern short int file_count;
extern short int dirent_count;
extern mp_pool_t mp_tree;
extern unsigned char array_tree[TREE_SIZE * MAX_TREE];

/*拷贝统计计数器(多个工作线程并发累加,用原子操作保证线程安全)*/
unsigned int copy_success_count = 0;
unsigned int copy_fail_count = 0;

tree *tree_init(const char *pathname)
{
    tree *t = mp_malloc(&mp_tree);
    if(t == NULL)
    {
        fprintf(stderr, "树内存分配失败\n");
        return NULL;
    }
    t->first_child = NULL;
    t->next_sibling = NULL;
    t->parent = NULL;
    t->layer = 0;
    t->is_directory = 1;
    strcpy(t->filename,pathname);
    struct stat st;
    if (stat(pathname, &st) == 0) {
        t->is_directory = S_ISDIR(st.st_mode);
    } else {
        // stat 失败：根据文件名特征推断
        char *p = strrchr(pathname, '.');
        if (p && strchr(p, '/') == NULL) {
            t->is_directory = 0;  // 有 . 且后面没有 / → 文件
        } else {
            t->is_directory = 1;  // 没有扩展名 → 目录
        }
    }
    return t;
}

void tree_child(tree *t,tree *new_t)
{
    new_t->layer += t->layer + 1;
    if(t->first_child == NULL)
    {
        t->first_child = new_t;
    }else{
        tree *p = t->first_child;
        while(p->next_sibling != NULL)
        {
            p = p->next_sibling;
        }
        p->next_sibling = new_t;
    }
}

/*安全拼接路径:先校验长度,超长返回-1并报错,避免截断产生错误路径*/
static int build_path(char *dst, size_t dst_size, const char *dir, const char *name)
{
    size_t dirlen = strlen(dir);
    size_t namelen = strlen(name);
    /*dir 是否以 '/' 结尾,若是则不再额外加分隔符,避免产生双斜杠(如根目录 '/')*/
    int has_slash = (dirlen > 0 && dir[dirlen - 1] == '/');
    size_t sep = has_slash ? 0 : 1;
    if(dirlen + sep + namelen + 1 > dst_size)
    {
        fprintf(stderr, "路径过长,已跳过: %s%s%s\n", dir, has_slash ? "" : "/", name);
        return -1;
    }
    snprintf(dst, dst_size, "%s%s%s", dir, has_slash ? "" : "/", name);
    return 0;
}

void *tree_build(void *vt)
{
    tree *root = vt;
    /*显式栈,放在堆上,避免递归导致的线程栈溢出*/
    tree **stack = malloc(sizeof(tree *) * MAX_TREE);
    if(stack == NULL)
    {
        fprintf(stderr, "遍历栈分配失败\n");
        return NULL;
    }
    int top = 0;
    stack[top++] = root;
    while(top > 0)
    {
        tree *t = stack[--top];
        DIR *dir = opendir(t->filename);
        if(dir == NULL)
        {
            continue;
        }
        struct dirent *dirent;
        while((dirent = readdir(dir)) != NULL)
        {
            /*跳过当前目录和上级目录*/
            if(strcmp(dirent->d_name,".")==0 || strcmp(dirent->d_name,"..")==0)
            {
                continue;
            }
            /*子目录*/
            if(dirent->d_type==DT_DIR)
            {
                char fullpath[MAX_FILENAME];
                /*先拼接并校验长度,超长跳过,避免截断产生错误路径*/
                if(build_path(fullpath, sizeof(fullpath), t->filename, dirent->d_name) != 0)
                {
                    continue;
                }
                dirent_count++;
                tree * new_t = tree_init("");
                if(new_t == NULL)
                {
                    continue;
                }
                tree_child(t,new_t);
                new_t->parent = t;
                strcpy(new_t->filename, fullpath);
                if(top < MAX_TREE)
                {
                    stack[top++] = new_t;/*待遍历的子目录入栈*/
                }
            }
            /*普通文件*/
            if(dirent->d_type==DT_REG)
            {
                char fullpath[MAX_FILENAME];
                /*先拼接并校验长度,超长跳过,避免截断产生错误路径*/
                if(build_path(fullpath, sizeof(fullpath), t->filename, dirent->d_name) != 0)
                {
                    continue;
                }
                file_count++;
                tree * new_t = tree_init("");
                if(new_t == NULL)
                {
                    continue;
                }
                new_t->is_directory = 0;
                new_t->parent = t;
                tree_child(t,new_t);
                /*拼接得到子目录的绝对路径*/
                strcpy(new_t->filename, fullpath);
            }
        }
        closedir(dir);
    }
    free(stack);
    return NULL;
}

/*打印树的分枝*/
void print_vertical(tree *t, int x)
{
    if (!t || t->layer == 0) return;
    
    /*动态分配,避免固定 100 层导致深目录数组越界*/
    int *has_sibling = malloc(sizeof(int) * (t->layer + 1));
    if(has_sibling == NULL)
    {
        return;
    }
    int count = 0;
    tree *cur = t;
    
    /*从当前节点往上回溯到根*/
    while (cur->parent) {
        has_sibling[count++] = (cur->parent->next_sibling != NULL);
        cur = cur->parent;
    }
    
    /*从根到当前节点的父节点打印*/
    for (int i = count - 1; i >= 0; i--) {
        printf("%s", has_sibling[i] ? "│   " : "    ");
    }
    free(has_sibling);
}

/*打印单个树节点(前缀+颜色),从 show_tree 的递归打印逻辑提取*/
static void print_tree_node(tree *t)
{
    char *str = strrchr(t->filename, '/');
    const char *name = str ? str + 1 : t->filename;
    print_vertical(t, t->layer);
    /*是否有后续兄弟决定用 └── 还是 ├──*/
    if(t->next_sibling == NULL)
    {
        PRINT_LAST;
    }
    else
    {
        PRINT_MID;
    }
    if(t->is_directory)
    {
        printf("%s%s%s%s\n", "\033[42m", "\033[34m", name, SE_CLOSE);
    }
    else
    {
        printf("%s%s%s\n", "\033[34m", name, SE_CLOSE);
    }
}

/*打印整颗树(显式栈迭代,避免递归栈溢出)*/
void show_tree(tree *root, int p)
{
    if (!root)
    {
        return;
    }
    /*显式栈,放在堆上,避免递归导致的线程栈溢出*/
    tree **stack = malloc(sizeof(tree *) * MAX_TREE);
    if(stack == NULL)
    {
        fprintf(stderr, "遍历栈分配失败\n");
        return;
    }
    int top = 0;
    stack[top++] = root;
    while(top > 0)
    {
        tree *t = stack[--top];
        print_tree_node(t);
        /*先入栈兄弟再入栈孩子(LIFO),保证 first_child 先被访问*/
        if(t->next_sibling != NULL)
        {
            stack[top++] = t->next_sibling;
        }
        if(t->first_child != NULL)
        {
            stack[top++] = t->first_child;
        }
    }
    free(stack);
}

void *copy_file(void *arg)
{
    file_path *path = (file_path *)arg;
    int src_fd = open(path->srcfile_path, O_RDONLY);
    if(src_fd == -1)
    {
        fprintf(stderr, "打开源文件失败: %s\n", path->srcfile_path);
        __atomic_fetch_add(&copy_fail_count, 1, __ATOMIC_SEQ_CST);
        free(path);
        return NULL;
    }
    int goal_fd = open(path->goalfile_path, O_WRONLY|O_CREAT|O_EXCL, 0777);
    if(goal_fd == -1)
    {
        fprintf(stderr, "创建目标文件失败: %s\n", path->goalfile_path);
        close(src_fd);
        __atomic_fetch_add(&copy_fail_count, 1, __ATOMIC_SEQ_CST);
        free(path);
        return NULL;
    }
    char buf[2048];
    ssize_t n;
    /*循环读写,处理 read 返回 0(EOF)与 -1(错误)*/
    while((n = read(src_fd, buf, sizeof(buf))) > 0)
    {
        ssize_t written = 0;
        /*write 可能短写,循环写完本次读到的所有字节*/
        while(written < n)
        {
            ssize_t w = write(goal_fd, buf + written, n - written);
            if(w == -1)
            {
                fprintf(stderr, "写入失败: %s\n", path->goalfile_path);
                close(src_fd);
                close(goal_fd);
                __atomic_fetch_add(&copy_fail_count, 1, __ATOMIC_SEQ_CST);
                free(path);
                return NULL;
            }
            written += w;
        }
    }
    if(n == -1)
    {
        fprintf(stderr, "读取失败: %s\n", path->srcfile_path);
        close(src_fd);
        close(goal_fd);
        __atomic_fetch_add(&copy_fail_count, 1, __ATOMIC_SEQ_CST);
        free(path);
        return NULL;
    }
    close(src_fd);
    close(goal_fd);
    __atomic_fetch_add(&copy_success_count, 1, __ATOMIC_SEQ_CST);
    free(path);
    return NULL;
}

/*显式栈帧:节点 + 其目标目录路径*/
typedef struct copy_frame
{
    tree *node;
    char goal[1024];
} copy_frame;

void tree_copy(tree *root, char *goal, thread_pool *pool)
{
    if (!root)
    {
        return ;
    }
    /*显式栈,放在堆上,避免递归导致的线程栈溢出*/
    copy_frame *stack = malloc(sizeof(copy_frame) * MAX_TREE);
    if(stack == NULL)
    {
        fprintf(stderr, "拷贝遍历栈分配失败\n");
        return ;
    }
    int top = 0;
    stack[top].node = root;
    strncpy(stack[top].goal, goal, sizeof(stack[top].goal) - 1);
    stack[top].goal[sizeof(stack[top].goal) - 1] = '\0';
    top++;

    while(top > 0)
    {
        copy_frame frame = stack[--top];
        tree *t = frame.node;
        char *g = frame.goal;

        /*创建目录(目录已存在不算错误)*/
        if(mkdir(g, 0777) == -1 && errno != EEXIST)
        {
            fprintf(stderr, "创建目录失败: %s\n", g);
        }

        /*取文件名(跳过最后一个 '/' )*/
        char *base = strrchr(t->filename, '/');
        base = base ? base + 1 : t->filename;
        char str[1024];
        /*g 是否以 '/' 结尾,若是则不再额外加分隔符,避免双斜杠*/
        size_t glen = strlen(g);
        int g_has_slash = (glen > 0 && g[glen - 1] == '/');
        snprintf(str, sizeof(str), "%s%s%s", g, g_has_slash ? "" : "/", base);

        if(!t->is_directory)
        {
            file_path *path = malloc(sizeof(file_path));
            if(path == NULL)
            {
                fprintf(stderr, "file_path 分配失败, 跳过: %s\n", t->filename);
                continue;
            }
            strcpy(path->srcfile_path, t->filename);
            strcpy(path->goalfile_path, str);
            printf("src:%s\n", path->srcfile_path);
            printf("goal:%s\n", path->goalfile_path);
            if(list_add_tail(copy_file, path, pool) != 0)
            {
                fprintf(stderr, "任务添加失败, 跳过: %s\n", t->filename);
                free(path);
            }
        }
        else
        {
            /*先入栈兄弟(goal 不变),再入栈孩子(goal=str),保证孩子先被处理*/
            if(t->next_sibling != NULL)
            {
                stack[top].node = t->next_sibling;
                strncpy(stack[top].goal, g, sizeof(stack[top].goal) - 1);
                stack[top].goal[sizeof(stack[top].goal) - 1] = '\0';
                top++;
            }
            if(t->first_child != NULL)
            {
                stack[top].node = t->first_child;
                strncpy(stack[top].goal, str, sizeof(stack[top].goal) - 1);
                stack[top].goal[sizeof(stack[top].goal) - 1] = '\0';
                top++;
            }
        }
    }
    free(stack);
}