/**
 * @author ttoobne
 * @date 2020/6/11
 */

#include "rbtree.h"

#include "log.h"

#include <stdlib.h>

static rbtree_node_t* rbtree_get_successor(rbtree_node_t* node, rbtree_node_t* nil);
static void rbtree_left_rotate(rbtree_node_t** root, rbtree_node_t* node, rbtree_node_t* nil);
static void rbtree_right_rotate(rbtree_node_t** root, rbtree_node_t* node, rbtree_node_t* nil);

/*
 * 红黑树的初始化函数。
 * nil 参数为 NIL 节点， insert 为 rbtree_insert_func 函数指针，可选择插入节点的方式。
 */
int rbtree_init(volatile rbtree_t* tree, rbtree_node_t* nil, rbtree_insert_func* insert) {
    if (tree == NULL) {
        log_error("rbtree ptr is NULL.");
        return -1;
    }

    rbtree_dye_black(nil);
    tree->root = nil;
    tree->nil = nil;
    tree->insert = insert;

    return 0;
}

/*
 * 获取红黑树中键最小的节点。
 */
rbtree_node_t* rbtree_get_min(volatile rbtree_t* tree) {
    rbtree_node_t* node;
    rbtree_node_t* nil;

    if (tree == NULL) {
        log_error("rbtree ptr is NULL.");
        return NULL;
    }
    
    node = tree->root;
    nil = tree->nil;

    if (node == nil) {
        return NULL;
    }

    while (node->left_son != nil) {
        node = node->left_son;
    }

    return node;
}

/*
 * 供 rbtree_delete 查找后继节点。
 */
static rbtree_node_t* rbtree_get_successor(rbtree_node_t* node, rbtree_node_t* nil) {
    node = node->right_son; /* 调用该函数时右儿子不会是 nil */

    while (node->left_son != nil) {
        node = node->left_son;
    }

    return node;
}

/*
 * 红黑树的查找。根据红黑树指定的插入方式进行查找。
 */
rbtree_node_t* rbtree_find(volatile rbtree_t* tree, rbtree_key_t key) {
    rbtree_node_t* now;
    rbtree_node_t* nil;
    rbtree_insert_func* insert;

    if (tree == NULL) {
        log_error("rbtree ptr is NULL.");
        return NULL;
    }

    now = tree->root;
    nil = tree->nil;
    insert = tree->insert;

    while (now != nil) {
        /* 通过有符号数比较进行的查找方式 */
        if (insert == rbtree_insert_value) {
            if ((rbtree_key_int_t)key < (rbtree_key_int_t)now->key) {
                now = now->left_son;
            } else if ((rbtree_key_int_t)key > (rbtree_key_int_t)now->key) {
                now = now->right_son;
            } else {
                return now;
            }
        } else if (insert == rbtree_insert_uvalue) {
            /* 通过无符号数比较进行的查找方式 */
            if (key < now->key) {
                now = now->left_son;
            } else if (key > now->key) {
                now = now->right_son;
            } else {
                return now;
            }
        } else if (insert == rbtree_insert_timer) {
            /* timer 的查找方式 */
            if ((rbtree_key_int_t)(key - now->key) < 0) {
                now = now->left_son;
            } else if ((rbtree_key_int_t)(key - now->key) > 0) {
                now = now->right_son;
            } else {
                return now;
            }
        }
    }

    return NULL;
}

/*
 * 插入节点的方式。判断大小时，键为有符号数。
 */
void rbtree_insert_value(rbtree_node_t* now, rbtree_node_t* node, rbtree_node_t* nil) {
    rbtree_node_t** ist;    /* 迭代二级指针，最后指向被插入点的地址 */
                            /* 必须为二级指针，因为已经赋值完的一级指针会随着函数返回而释放 */

    /* 普通二叉搜索树的插入 */
    for ( ;; ) {
        /* 判断大小时，键为有符号数 */
        if ((rbtree_key_int_t)node->key < (rbtree_key_int_t)now->key) {
            ist = &(now->left_son);
        } else {
            ist = &(now->right_son);
        }

        if (*ist == nil) {
            break;
        }

        now = *ist;
    }

    *ist = node;

    /* 初始化节点基本信息 */
    node->parent = now;
    node->left_son = nil;
    node->right_son = nil;
    rbtree_dye_red(node);
}

/*
 * 插入节点的方式。判断大小时，键为无符号数。
 */
void rbtree_insert_uvalue(rbtree_node_t* now, rbtree_node_t* node, rbtree_node_t* nil) {
    rbtree_node_t** ist;    /* 迭代二级指针，最后指向被插入点的地址 */
                            /* 必须为二级指针，因为已经赋值完的一级指针会随着函数返回而释放 */

    /* 普通二叉搜索树的插入 */
    for ( ;; ) {
        /* 判断大小时，键为无符号数 */
        if (node->key < now->key) {
            ist = &(now->left_son);
        } else {
            ist = &(now->right_son);
        }

        if (*ist == nil) {
            break;
        }

        now = *ist;
    }

    *ist = node;

    /* 初始化节点基本信息 */
    node->parent = now;
    node->left_son = nil;
    node->right_son = nil;
    rbtree_dye_red(node);
}

/*
 * 插入节点的方式。用 timer 特定的方式判断大小。
 */
void rbtree_insert_timer(rbtree_node_t* now, rbtree_node_t* node, rbtree_node_t* nil) {
    rbtree_node_t** ist;    /* 迭代二级指针，最后指向被插入点的地址 */
                            /* 必须为二级指针，因为已经赋值完的一级指针会随着函数返回而释放 */

    /* 普通二叉搜索树的插入 */
    for ( ;; ) {
        /* 考虑如果使用 timer 时溢出 */
        /* 就算是 32 位操作系统，也将有 2^31 大小的缓冲空间 */
        /* (在这个空间内溢出的值计算后仍大于未溢出的值) */
        /* 对于定时器来说使用跨度不会超过这个范围(单位为 ms ) */
        if ((rbtree_key_int_t)(node->key - now->key) < 0) {
            ist = &(now->left_son);
        } else {
            ist = &(now->right_son);
        }

        if (*ist == nil) {
            break;
        }

        now = *ist;
    }

    *ist = node;

    /* 初始化节点基本信息 */
    node->parent = now;
    node->left_son = nil;
    node->right_son = nil;
    rbtree_dye_red(node);
}

/* 
 * 平衡树的左旋操作，左旋后依然保持二叉搜索顺序。
 *       ...                ...
 *        |                  |
 *        1                  3
 *      /   \      ->      /   \
 *    ...    3            1     4
 *          /  \        /  \    |
 *         2    4     ...   2  ...
 *         |    |           |
 *        ...  ...         ...
 * 图中数字为 key (键)，参数中 node 节点所处的位置为左图 key 为 1 的点。
 */
static void rbtree_left_rotate(rbtree_node_t** root, rbtree_node_t* node, rbtree_node_t* nil) {
    rbtree_node_t* temp;

    temp = node->right_son; /* 保存 temp 指向 node 的右孩子 */

    /* 更新旋转后 temp 与其新父亲的关系 */
    temp->parent = node->parent;
    if (node == *root) {
        *root = temp;
    } else if (node == node->parent->left_son) {
        node->parent->left_son = temp;
    } else {
        node->parent->right_son = temp;
    }
    
    /* 设置 node 的右孩子为 temp 的左孩子 */
    node->right_son = temp->left_son;
    if (node->right_son != nil) {
        node->right_son->parent = node;
    }
    
    /* 更新旋转后 node 与其新父亲的关系 */
    node->parent = temp;
    node->parent->left_son = node;
}

/*
 * 平衡树的左旋操作，左旋后依然保持二叉搜索顺序。
 *       ...                ...
 *        |                  |
 *        4                  2
 *      /   \              /   \
 *     2    ...    ->     1     4
 *    / \                 |    /  \
 *   1   3               ...  3   ...
 *   |   |                    |
 *  ... ...                  ...
 * 图中数字为 key (键)，参数中 node 节点所处的位置为左图 key 为 4 的点。
 */
static void rbtree_right_rotate(rbtree_node_t** root, rbtree_node_t* node, rbtree_node_t* nil) {
    rbtree_node_t* temp;

    temp = node->left_son;

    /* 更新旋转后 temp 与其新父亲的关系 */
    temp->parent = node->parent;
    if (node == *root) {
        *root = temp;
    } else if (node == node->parent->left_son) {
        node->parent->left_son = temp;
    } else {
        node->parent->right_son = temp;
    }

    /* 设置 node 的左孩子为 temp 的右孩子 */
    node->left_son = temp->right_son;
    if (node->left_son != nil) {
        node->left_son->parent = node;
    }

    /* 更新旋转后 node 与其新父亲的关系 */
    node->parent = temp;
    node->parent->right_son = node;
}

/*
 * 红黑树插入节点。
 */
int rbtree_insert(volatile rbtree_t* tree, rbtree_node_t* node) {
    rbtree_node_t** root;
    rbtree_node_t* nil;
    rbtree_node_t* uncle;

    if (tree == NULL) {
        log_error("rbtree ptr is NULL.");
        return -1;
    }

    root = (rbtree_node_t**)&(tree->root);
    nil = tree->nil;
    
    /* 如果红黑树为空 */
    if (*root == nil) {
        *root = node;
        node->left_son = nil;
        node->right_son = nil;
        node->parent = NULL;
        rbtree_dye_black(node);

        return 0;
    }
    
    tree->insert(*root, node, nil);

    /*
     * 红黑树的性质（引自维基百科）
     *  1.节点是红色或黑色。
     *  2.根是黑色。
     *  3.所有叶子都是黑色（叶子是NIL节点）。
     *  4.每个红色节点必须有两个黑色的子节点。（从每个叶子到根的所有路径上不能有两个连续的红色节点。）
     *  5.从任一节点到其每个叶子的所有简单路径都包含相同数目的黑色节点。
     */
    /* 以下的任何操作都不会破坏性质 5 ，但可能导致性质 4 被破坏 */

    /* 如果父亲节点是红色的则性质 4 依然没有满足 */
    while (node != *root && rbtree_is_red(node->parent)) {
        /* 此处父亲节点一定不是根 */
        /* 如果叔叔节点是祖父节点的右孩子 */
        if (node->parent == node->parent->parent->left_son) {
            uncle = node->parent->parent->right_son;

            /* 如果叔叔节点是红色的 */
            if (rbtree_is_red(uncle)) {
                /* 此处可能导致性质 4 被破坏，如果破坏需要继续向上维护 */
                /* case 1: 叔叔和父亲都是红色 */
                /* 将叔叔和父亲染黑，祖父染红，那么祖父可能使得性质破坏 */
                rbtree_dye_black(node->parent);
                rbtree_dye_black(uncle);
                rbtree_dye_red(node->parent->parent);
                node = node->parent->parent;
            } else {
                /* 此处完成后已经满足所有性质 */
                /* case 2: node 是父亲节点的右孩子 */
                /* 通过左旋转化为 case 3 */
                if (node == node->parent->right_son) {
                    node = node->parent;
                    rbtree_left_rotate(root, node, nil);
                }

                /* case 3: 父亲是红色，叔叔是黑色 */
                /* 将父亲染黑，祖父染红，在祖父处进行右旋 */
                rbtree_dye_black(node->parent);
                rbtree_dye_red(node->parent->parent);
                rbtree_right_rotate(root, node->parent->parent, nil);
                /* 至此已经满足所有性质 */
            }
        } else {
            /* 如果叔叔节点是祖父节点的左孩子 */
            uncle = node->parent->parent->left_son;
            /* 如果叔叔节点是红色的 */
            if (rbtree_is_red(uncle)) {
                /* 此处可能导致性质 4 被破坏，如果破坏需要继续向上维护 */
                /* case 4: 叔叔和父亲都是红色 */
                /* 将叔叔和父亲染黑，祖父染红，那么祖父可能使得性质破坏 */
                rbtree_dye_black(uncle);
                rbtree_dye_black(node->parent);
                rbtree_dye_red(node->parent->parent);
                node = node->parent->parent;
            } else {
                /* 此处完成后已经满足所有性质 */
                /* case 5: node 是父亲节点的左孩子 */
                /* 通过右旋转化为 case 6 */
                if (node == node->parent->left_son) {
                    node = node->parent;
                    rbtree_right_rotate(root, node, nil);
                }

                /* case 6: 父亲是红色，叔叔是黑色 */
                /* 将父亲染黑，祖父染红，在祖父处进行左旋 */
                rbtree_dye_black(node->parent);
                rbtree_dye_red(node->parent->parent);
                rbtree_left_rotate(root, node->parent->parent, nil);
            }
        }
    }

    /* 如果最后 node 到了根节点的位置 */
    rbtree_dye_black(*root);

    return 0;
}

/*
 * 红黑树删除节点。
 */
int rbtree_delete(volatile rbtree_t* tree, rbtree_node_t* node) {
    rbtree_node_t** root;
    rbtree_node_t* nil;
    rbtree_node_t* replace;
    rbtree_node_t* inharm;
    rbtree_node_t* brother;
    int red;

    if (tree == NULL) {
        log_error("rbtree ptr is NULL.");
        return -1;
    }

    if (node == NULL) {
        log_error("rbtree node is invalid.");
        return -1;
    }

    root = (rbtree_node_t**)&(tree->root);
    nil = tree->nil;

    /*
     * 红黑树的删除操作整体分为两部分
     * 第一部分类似普通二叉搜索树的删除。
     * 如果只有一个孩子（非叶子节点），那么直接让该孩子替代被删除点的位置即可。
     * 如果有两个孩子（非叶子节点），那么找到该节点的前驱或者后继节点，将该点的值赋给被删点，从而替代要删的点被删除。
     * 这里是用的后继节点。
     */

    /*
     * Delete 部分
     * 类似普通二叉搜索树那样删除节点
     */

    if (node->left_son == nil) {
        /*
         * D-case 1: 有一个右孩子的简单情况（这里孩子指非叶子节点）
         * 将右孩子（一定是红色）变黑替代 node 即可。
         */
        /*
         * D-case 2: 没有孩子的情况（这里孩子指非叶子节点）
         * 如果 node 是红色那么不需要变色
         * 如果 node 为黑色则最为复杂，需要执行第二部分 Fix 操作
         */
        replace = node;
        inharm = node->right_son;
    } else if (node->right_son == nil) {
        /*
         * D-case 3: 有一个左孩子的情况
         * 将左孩子（一定是红色）变黑替代 node 即可。
         */
        replace = node;
        inharm = node->left_son;
    } else {
        /*
         * D-case 4: 有两个孩子（这里孩子指非叶子节点）
         * 可以通过用后继节点替换 node 从而转化为 D-case 1~3
         * 因为后继节点一定只有一个孩子
         */
        replace = rbtree_get_successor(node, nil);
        inharm = replace->right_son;
    }

    /* D-case 1~3 中如果 node 是 root */
    if (replace == *root) {
        inharm->parent = NULL;
        *root = inharm;
        rbtree_dye_black(*root);

        /* 清理工作实际上由调用者完成，这里简单清空指针 */
        node->left_son = NULL;
        node->right_son = NULL;
        node->parent = NULL;

        return 0;
    }

    /* 由于 node 的内存由调用者管理，所以用 replace 接替 node 的位置而非直接将 replace 的值赋给 node */
    /* 那么需要保存 replace 的颜色(也就是删掉的点的颜色) */
    red = rbtree_is_red(replace);

    if (replace == node) {
        /* D-case 1~3 */
        /* 直接让 node 的儿子接替 node 即可 */
        inharm->parent = node->parent;
        if (node == node->parent->left_son) {
            node->parent->left_son = inharm;
        } else {
            node->parent->right_son = inharm;
        }
    } else {
        /* D-case 4 转化为 D-case 1~3 */
        /* 由 replace 替代 node 被删除 */
        /* 这里由于 node 的内存由调用者管理，所以并不是直接将 replace 的值放到 node */
        /* 而是用 replace 接替 node 的位置 */
        if (replace->parent != node) {
            /* 如果 replace 与 node 之间隔了若干代 */
            inharm->parent = replace->parent;
        } else {
            inharm->parent = replace;
        }

        /* 不管 replace 的父亲是不是 node 都要替换 replace 父亲的孩子为 inharm */
        if (replace == replace->parent->left_son) {
            replace->parent->left_son = inharm;
        } else {
            replace->parent->right_son = inharm;
        }

        /* 用 replace 替换 node 的位置和颜色 */
        replace->left_son = node->left_son;
        replace->right_son = node->right_son;
        replace->parent = node->parent;
        rbtree_copy_color(replace, node);

        /* 更新父亲的孩子 */
        if (node == *root) {
            *root = replace;
        } else {
            if (node == node->parent->left_son) {
                node->parent->left_son = replace;
            } else {
                node->parent->right_son = replace;
            }
        }

        /* 更新孩子的父亲 */
        if (replace->left_son != nil) {
            replace->left_son->parent = replace;
        }
        if (replace->right_son != nil) {
            replace->right_son->parent = replace;
        }
    }

    /* 清理工作实际上由调用者完成，这里简单清空指针 */
    node->left_son = NULL;
    node->right_son = NULL;
    node->parent = NULL;

    /* 如果被删除的点是红色，那么不需要做任何处理 */
    if (red) {
        return 0;
    }

    /*
     * 红黑树的性质（引自维基百科）
     *  1.节点是红色或黑色。
     *  2.根是黑色。
     *  3.所有叶子都是黑色（叶子是NIL节点）。
     *  4.每个红色节点必须有两个黑色的子节点。（从每个叶子到根的所有路径上不能有两个连续的红色节点。）
     *  5.从任一节点到其每个叶子的所有简单路径都包含相同数目的黑色节点。
     */

    /*
     * Fix 部分
     * 删除完点之后如果被删点是黑色，则所在链上不满足性质 5 ，需要调整
     * D-case 2 较为复杂，可能需要向上调整
     * 而 D-case 1,3 直接改变 inharm 颜色即可
     */

    /* D-case 2 由于删除了一个黑色节点导致 inharm 所在的链不满足性质 5 */
    /* 则需要不断向上调整 */
    /* 此时 inharm 一定是 nil 节点，且一定有一个非 nil 的兄弟 */
    while (inharm != *root && rbtree_is_black(inharm)) {
        /* 如果兄弟节点是右孩子 */
        if (inharm == inharm->parent->left_son) {
            brother = inharm->parent->right_son;
            if (brother == nil) {
                log_debug("brother is nil");
                exit(1);
            }
            //ASSERT(brother != nil, "why!!! brother is nil!\n");

            /* F-case 1: */
            /* 如果兄弟为红色，那么 inharm 父亲一定为黑色 */
            /* 将兄弟和父亲变色，然后左旋则兄弟节点就是黑色，则转化为了 F-case 2~4 */
            if (rbtree_is_red(brother)) {
                rbtree_dye_black(brother);
                rbtree_dye_red(inharm->parent);
                rbtree_left_rotate(root, inharm->parent, nil);
                brother = inharm->parent->right_son;
            }

            if (brother == nil) {
                log_debug("brother is nil after f-case 1");
                exit(1);
            }

            /* F-case 2: */
            /* 如果兄弟节点和它的两个孩子都是黑色 */
            /* 那么兄第节点可以变成红色来弥补不平衡的黑色 */
            if (rbtree_is_black(brother->left_son) && rbtree_is_black(brother->right_son)) {
                rbtree_dye_red(brother);
                inharm = brother->parent;
                /* 如果父亲是是红色则退出循环再将 inharm 染黑即可满足所有性质 */
                /* 否则是黑色就需要继续向上调整 */
            } else {
                /* F-case 3: */
                /* 如果兄弟节点的右孩子不是红色，那么左孩子一定是红色 */
                /* 将兄弟节点和其左孩子变色，进行右旋，这时新兄弟节点右孩子是红色，转化为 F-case 4 */
                if (rbtree_is_black(brother->right_son)) {
                    rbtree_dye_red(brother);
                    rbtree_dye_black(brother->left_son);
                    rbtree_right_rotate(root, brother, nil);
                    brother = inharm->parent->right_son;
                }

                if (brother == nil) {
                    log_debug("brother is nil in f-case 4");
                    exit(1);
                }

                /* F-case 4: */
                /* 兄弟节点的右孩子为红色 */
                /* 这时要用右孩子变黑来弥补左边失去的黑色 */
                rbtree_dye_black(brother->right_son);
                rbtree_copy_color(brother, brother->parent);
                rbtree_dye_black(brother->parent);
                rbtree_left_rotate(root, brother->parent, nil);
                inharm = *root;     /* 至此已经满足所有性质 */
            }
        } else {
            /* 如果兄弟节点是左孩子 */
            brother = inharm->parent->left_son;

            /* F-case 5: */
            /* 如果兄弟为红色，那么 inharm 父亲一定为黑色 */
            /* 将兄弟和父亲变色，然后右旋则兄弟节点就是黑色，则转化为了 F-case 6~8 */
            if (rbtree_is_red(brother)) {
                rbtree_dye_black(brother);
                rbtree_dye_red(inharm->parent);
                rbtree_right_rotate(root, inharm->parent, nil);
                brother = inharm->parent->left_son;
            }

            /* F-case 6: */
            /* 如果兄弟节点和它的两个孩子都是黑色 */
            /* 那么兄第节点可以变成红色来弥补不平衡的黑色 */
            if (rbtree_is_black(brother->left_son) && rbtree_is_black(brother->right_son)) {
                rbtree_dye_red(brother);
                inharm = brother->parent;
                /* 如果父亲是是红色则退出循环再将 inharm 染黑即可满足所有性质 */
                /* 否则是黑色就需要继续向上调整 */
            } else {
                /* F-case 7: */
                /* 如果兄弟节点的左孩子不是红色，那么右孩子一定是红色 */
                /* 将兄弟节点和其右孩子变色，进行左旋，这时新兄弟节点左孩子是红色，转化为 F-case 8 */
                if (rbtree_is_black(brother->left_son)) {
                    rbtree_dye_red(brother);
                    rbtree_dye_black(brother->right_son);
                    rbtree_left_rotate(root, brother, nil);
                    brother = inharm->parent->left_son;
                }

                /* F-case 8: */
                /* 兄弟节点的左孩子为红色 */
                /* 这时要用左孩子变黑来弥补右边失去的黑色 */
                rbtree_dye_black(brother->left_son);
                rbtree_copy_color(brother, brother->parent);
                rbtree_dye_black(brother->parent);
                rbtree_right_rotate(root, brother->parent, nil);
                inharm = *root;     /* 至此已经满足所有性质 */
            }
        }
    }
    
    /* 如果是 D-case 1,3 直接将 inharm 染黑即可 */
    /* 如果是 F-case 2,6 也是直接将 inharm 染黑 */
    rbtree_dye_black(inharm);

    return 0;
}