/**
 * @author ttoobne
 * @date 2020/6/11
 */

/*
 * 部分定义和实现参考自 nginx 源码。
 */

#ifndef _RBTREE_H_
#define _RBTREE_H_

typedef unsigned long int   rbtree_key_t;
typedef long int            rbtree_key_int_t;

/* 红黑树节点 */
typedef struct rbtree_node_s rbtree_node_t;
struct rbtree_node_s {
    rbtree_key_t            key;        /* 键 */
    unsigned char           color;      /* 红黑树颜色 */
    rbtree_node_t*          left_son;   /* 左孩子 */
    rbtree_node_t*          right_son;  /* 右孩子*/
    rbtree_node_t*          parent;     /* 父亲节点 */
    void*                   value;      /* 值 */
};

typedef void (rbtree_insert_func) (rbtree_node_t* root, rbtree_node_t* node, rbtree_node_t* nil);

/* 红黑树 */
typedef struct {
    rbtree_node_t*          root;       /* 根节点 */
    rbtree_node_t*          nil;        /* 叶子节点（NIL节点） */
    rbtree_insert_func*     insert;     /* 插入节点的方式 */
} rbtree_t;

/* 给节点染色， 1 为红色， 0 为黑色 */
#define rbtree_dye_red(rbtree_node)     ((rbtree_node)->color = 1)
#define rbtree_dye_black(rbtree_node)   ((rbtree_node)->color = 0)

/* 判断红黑树的颜色 */
#define rbtree_is_red(rbtree_node)      ((rbtree_node)->color)
#define rbtree_is_black(rbtree_node)    (!rbtree_is_red(rbtree_node))

/* 复制节点颜色 */
#define rbtree_copy_color(n1, n2)       ((n1)->color = (n2)->color)

/*
 * 红黑树的初始化函数。
 * nil 参数为 NIL 节点， insert 为 rbtree_insert_func 函数指针，可选择插入节点的方式。
 */
int rbtree_init(volatile rbtree_t* tree, rbtree_node_t* nil, rbtree_insert_func* insert);

/*
 * 获取红黑树中键最小的节点。
 */
rbtree_node_t* rbtree_get_min(volatile rbtree_t* tree);

/*
 * 红黑树的查找。根据红黑树指定的插入方式进行查找。
 */
rbtree_node_t* rbtree_find(volatile rbtree_t* tree, rbtree_key_t key);

/*
 * 插入节点的方式。判断大小时，键为有符号数。
 */
void rbtree_insert_value(rbtree_node_t* root, rbtree_node_t* node, rbtree_node_t* nil);

/*
 * 插入节点的方式。判断大小时，键为无符号数。
 */
void rbtree_insert_uvalue(rbtree_node_t* root, rbtree_node_t* node, rbtree_node_t* nil);

/*
 * 插入节点的方式。用 timer 特定的方式判断大小。
 */
void rbtree_insert_timer(rbtree_node_t* root, rbtree_node_t* node, rbtree_node_t* nil);

/*
 * 红黑树插入节点。
 */
int rbtree_insert(volatile rbtree_t* tree, rbtree_node_t* node);

/*
 * 红黑树删除节点。
 */
int rbtree_delete(volatile rbtree_t* tree, rbtree_node_t* node);

#endif /* _RBTREE_H_ */