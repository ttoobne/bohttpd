/**
 * @author ttoobne
 * @date 2020/6/4
 */

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <pthread.h>

typedef void* (task_function_t)(void*);      /* 简化函数类型 */

/*
 * 线程池任务类型。
 */
typedef struct {
    task_function_t*    func;               /* 指向任务函数 */
    void*               args;               /* 传入 func 的参数 */
} threadpool_task_t;

/*
 * 线程池类型。
 */
typedef struct {
    pthread_t*          threads;            /* 线程 tid 数组 */
    threadpool_task_t*  task_queue;         /* 任务队列，采用循环队列结构 */

    int                 threadpool_size;    /* 线程池大小 */
    int                 thread_running;     /* 运行中的线程数量，正常情况下等于线程池大小 */
    int                 task_queue_size;    /* 任务队列大小 */
    int                 task_queue_front;   /* 任务队列头部 */
    int                 task_queue_rear;    /* 任务队列尾部 */
    int                 task_num;           /* 任务队列中实际的任务数量 */
    int                 shutdown;           /* 线程池的关闭状态， 1 为关闭 */

    pthread_mutex_t     lock_mutex;         /* 用于内部工作的互斥锁 */
    pthread_cond_t      nfull_cond;         /* 任务队列非满的条件变量 */
    pthread_cond_t      nempty_cond;        /* 任务队列非空的条件变量 */
} threadpool_t;

/*
 * 创建线程池，线程池大小为 threadpool_size ，任务队列大小为 task_queue_size 。
 * 创建成功则返回相应线程池指针，否则返回 NULL 。
 */
threadpool_t* threadpool_create(int threadpool_size, int task_queue_size);
/*
 * 向线程池中添加任务。
 * 添加成功返回 0 ，否则返回 -1 。
 */
int threadpool_add_task(threadpool_t* threadpool, task_function_t* func, void* args);
/*
 * 销毁线程池。
 * 成功返回 0 ，否则返回 -1 。
 */
int threadpool_destroy(threadpool_t* threadpool);

#endif /* _THREADPOOL_H_ */
