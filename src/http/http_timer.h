/**
 * @author ttoobne
 * @date 2020/6/15
 */

#ifndef _HTTP_TIMER_H_
#define _HTTP_TIMER_H_

#include "rbtree.h"

#define TIMER_INFINITE      -1          /* 一直阻塞 */

typedef rbtree_key_t        msec_t;
typedef rbtree_key_int_t    msec_int_t;

typedef int (timer_handler_t) (void*);

typedef struct {
    rbtree_node_t       timer_node;     /* 红黑树中的节点 */
    timer_handler_t*    handler;        /* 超时事件的回调函数 */
    unsigned            timer_set:1;    /* 如果为 1 ，则该事件设置了定时器 */
    unsigned            timeout:1;      /* 如果为 1 ，表示事件已超时 */
} http_timer_t;

/*
 * 初始化定时器。
 */
int init_timer();

/*
 * 寻找定时器中距离超时时间最近的一个时间并返回。
 * 如果定时器为空则返回 TIMER_INFINITE 。
 */
msec_t find_timer();

/*
 * 执行超时事件的回调函数。
 */
int expire_timers();

/*
 * 为事件添加定时器。
 */
int add_timer(void* http_request, msec_t timeout, timer_handler_t* handler);

/*
 * 删除指定事件的定时器。
 */
int delete_timer(void* http_request);

#endif /* _HTTP_TIMER_H_ */