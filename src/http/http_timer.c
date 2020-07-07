/**
 * @author ttoobne
 * @date 2020/6/17
 */

#include "http_timer.h"

#include "log.h"
#include "http_request.h"

#include <sys/time.h>
#include <stddef.h>
#include <pthread.h>

static volatile rbtree_t    timer_rbtree;   /* 定时器所使用的红黑树 */
static rbtree_node_t        timer_nil;      /* 红黑树的叶子节点 */
static msec_t               current_msec;   /* 当前时间 */
static pthread_mutex_t      timer_mutex;    /* 用于同步的互斥锁 */

static int update_current_msec();

/*
 * 初始化定时器。
 */
int init_timer() {
    /* 初始化的主要工作就是初始化一颗红黑树 */
    if (rbtree_init(&timer_rbtree, &timer_nil, rbtree_insert_timer) != 0) {
        log_error("rbtree init error.");
        return -1;
    }

    /* 初始化互斥锁，用于多线程使用定时器的同步操作 */
    if (pthread_mutex_init(&timer_mutex, NULL) != 0) {
        log_error("timer mutex init failed.");
        return -1;
    }

    /* 初始化 current_msec ，可做可不做 */
    if (update_current_msec() != 0) {
        log_error("get current time error.");
        return -1;
    }

    return 0;
}

/*
 * 更新 current_msec 为当前时间，单位毫秒。
 */
static int update_current_msec() {
    struct timeval cur_time;
    
    if (gettimeofday(&cur_time, NULL) != 0) {
        return -1;
    }

    current_msec = cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000;

    return 0;
}

/*
 * 寻找定时器中距离超时时间最近的一个时间并返回。
 * 如果定时器为空则返回 TIMER_INFINITE 。
 */
msec_t find_timer() {
    msec_int_t timer;
    rbtree_node_t* node;

    //DBG("find timer pid:%u ready to lock\n", pthread_self());
    pthread_mutex_lock(&timer_mutex);
    //DBG("find timer pid:%u locked\n", pthread_self());

    /* 如果定时器为空，则返回无穷 */
    if (timer_rbtree.root == &timer_nil) {
        //DBG("find timer pid:%u ready to unlock\n", pthread_self());
        pthread_mutex_unlock(&timer_mutex);
        //DBG("find timer pid:%u unlocked\n", pthread_self());
        return TIMER_INFINITE;
    }

    /* 获取红黑树中最左边的节点即键最小的节点 */
    node = rbtree_get_min(&timer_rbtree);
    //ASSERT(node != NULL, "node is NULL.");

    //DBG("find timer pid:%u ready to unlock\n", pthread_self());
    pthread_mutex_unlock(&timer_mutex);
    //DBG("find timer pid:%u unlocked\n", pthread_self());

    /* 计算与当前时间的差值并返回 */
    update_current_msec();

    timer = (msec_int_t)(node->key - current_msec);

    return (msec_t)(timer > 0 ? timer : 0);
}

/*
 * 执行超时事件的回调函数。
 */
int expire_timers() {
    rbtree_node_t* node;
    http_timer_t* timer;
    http_request_t* ev;

    //while (timer_rbtree.root != &timer_nil) {
    for ( ;; ) {
        //DBG("expire timer pid:%u ready to lock\n", pthread_self());
        pthread_mutex_lock(&timer_mutex);

        //DBG("expire timer pid:%u locked\n", pthread_self());
        
        /* 定时器为空，即红黑树为空 */
        if (timer_rbtree.root == &timer_nil) {
            break;
        }

        /* 获取红黑树中最左边的节点即键最小的节点 */
        if ((node = rbtree_get_min(&timer_rbtree)) == NULL) {
            log_error("get min failed.");
            pthread_mutex_unlock(&timer_mutex);
            return -1;
        }

        update_current_msec();

        /* 计算是否超时 */
        if ((msec_int_t)(node->key - current_msec) <= 0) {
            /* 如果超时 */
            /* 删除红黑树中的节点 */
            rbtree_delete(&timer_rbtree, node);

            pthread_mutex_unlock(&timer_mutex);


            /* 通过偏移量获取 timer 指针 */
            timer = (http_timer_t*) ((char*)node - offsetof(http_timer_t, timer_node));

            /* 标记红黑树中已经不监控该事件 */
            timer->timer_set = 0;

            /* 通过偏移量获取事件指针 */
            ev = (http_request_t*) ((char*)timer - offsetof(http_request_t, timer));
            
            /* 标记事件已超时 */
            timer->timeout = 1;

            /* 执行回调函数 */
            if (timer->handler) {
                timer->handler((void*)ev);
            }

            continue;
        }


        break;
    }

    pthread_mutex_unlock(&timer_mutex);

    return 0;
}

/*
 * 为事件添加定时器。
 */
int add_timer(void* http_request, msec_t timeout, timer_handler_t* handler) {
    http_request_t* ev;
    http_timer_t* timer;

    if (http_request == NULL) {
        log_error("event ptr is NULL.");
        return -1;
    }

    if (timeout <= 0) {
        log_error("timeout is invalid.");
        return -1;
    }

    ev = (http_request_t*)http_request;
    timer = &(ev->timer);

    if (timer->timer_set) {
        /* 如果定时器已被设置，则先删除再添加 */
        //log_debug("delete timer first?");
        delete_timer(ev);
    }

    /* 设置标记位和回调函数 */
    timer->timer_set = 1;
    timer->timeout = 0;
    timer->handler = handler;

    update_current_msec();

    /* 设置超时时间 */
    timer->timer_node.key = current_msec + timeout;

    pthread_mutex_lock(&timer_mutex);

    /* 向红黑树中插入节点 */
    //log_debug("ready to insert:%d", timer->timer_node.key);
    if (rbtree_insert(&timer_rbtree, &(timer->timer_node)) != 0) {
        log_error("rbtree insert failed.");
        pthread_mutex_unlock(&timer_mutex);
        return -1;
    }

    pthread_mutex_unlock(&timer_mutex);

    return 0;
}

/*
 * 删除指定事件的定时器。
 */
int delete_timer(void* http_request) {
    http_request_t* ev;

    if (http_request == NULL) {
        log_error("event ptr is NULL.");
        return -1;
    }

    ev = (http_request_t*)http_request;

    pthread_mutex_lock(&timer_mutex);

    /* 删除红黑树中的节点 */
    //log_debug("ready to delete:%d", (ev->timer).timer_node.key);
    if (rbtree_delete(&timer_rbtree, &((ev->timer).timer_node)) != 0) {
        log_error("delete rbtree node failed.");
        pthread_mutex_unlock(&timer_mutex);
        return -1;
    }

    pthread_mutex_unlock(&timer_mutex);

    /* 标记红黑树中已经不监控该事件 */
    (ev->timer).timer_set = 0;

    return 0;
}