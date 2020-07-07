/**
 * @author ttoobne
 * @date 2020/6/4
 */

#include "threadpool.h"

#include "log.h"

#include <stdlib.h>

static void* threadpool_worker(void* threadpool);
static int threadpool_free(threadpool_t *threadpool);

/*
 * 创建线程池，线程池大小为 threadpool_size ，任务队列大小为 task_queue_size 。
 * 创建成功则返回相应线程池指针，否则返回 NULL 。
 */
threadpool_t* threadpool_create(int threadpool_size, int task_queue_size) {

    int i;
    threadpool_t* pool = NULL;

    if (threadpool_size <= 0 || task_queue_size <= 0) {
        log_error("arguments invalid.");
        return NULL;
    }

    do {
        if ((pool = (threadpool_t*)malloc(sizeof(threadpool_t))) == NULL) {
            log_error("malloc threadpool failed.");
            break;
        }

        /* 初始化互斥锁和条件变量 */
        if (pthread_mutex_init(&(pool->lock_mutex), NULL) != 0 ||
            pthread_cond_init(&(pool->nfull_cond), NULL) != 0 ||
            pthread_cond_init(&(pool->nempty_cond), NULL) != 0) {
            log_error("mutex and condition initialize failed.");
            break;
        }

        if ((pool->threads = (pthread_t*)malloc
            (sizeof(pthread_t) * threadpool_size)) == NULL) {
            log_error("malloc threads failed.");
            break;
        }

        if ((pool->task_queue = (threadpool_task_t*)malloc
            (sizeof(threadpool_task_t) * task_queue_size)) == NULL) {
            log_error("malloc task queue failed.");
            break;
        }

        /* 初始化各个属性值 */
        pool->threadpool_size = threadpool_size;
        pool->thread_running = 0;
        pool->task_queue_size = task_queue_size;
        pool->task_queue_front = 0;
        pool->task_queue_rear = 0;
        pool->task_num = 0;
        pool->shutdown = 0;

        /* 创建 threadpool_size 个线程 */
        for (i = 0; i < threadpool_size; ++ i) {
            if (pthread_create(&(pool->threads[i]), NULL,
                                threadpool_worker, (void*)pool) != 0) {
                log_error("create threads failed.");
                break;
            }
            pool->thread_running ++ ;
        }
        if (i != threadpool_size) {
            break;
        }

        return pool;

    } while (0);

    threadpool_free(pool);

    return NULL;
}

/*
 * 线程池中的工作线程将执行此函数。
 */
static void* threadpool_worker(void* threadpool) {

    threadpool_t* pool = (threadpool_t*)threadpool;
    threadpool_task_t task;

    for ( ;; ) {
        /* 获取互斥锁 */
        pthread_mutex_lock(&(pool->lock_mutex));

        /* 当任务队列中任务数为空，工作线程阻塞在 nempty_cond 上 */
        while ((pool->task_num == 0) && (!pool->shutdown)) {
            pthread_cond_wait(&(pool->nempty_cond), &(pool->lock_mutex));
        }
        
        /* 若线程池关闭，则线程终止 */
        if (pool->shutdown) {
            break;
        }

        /* 从任务队列首部取任务 */
        task.func = pool->task_queue[pool->task_queue_front].func;
        task.args = pool->task_queue[pool->task_queue_front].args;
        
        /* 循环队列 */
        pool->task_queue_front = (pool->task_queue_front + 1) % pool->task_queue_size;
        pool->task_num -- ;

        /* 取完任务则广播任务队列未满条件 */
        pthread_cond_broadcast(&(pool->nfull_cond));
        /* 解开互斥锁 */
        pthread_mutex_unlock(&(pool->lock_mutex));

        /* 执行任务 */
        (*(task.func))(task.args);
    }

    /* 终止线程 */
    pool->thread_running -- ;
    pthread_mutex_unlock(&(pool->lock_mutex));
    pthread_exit(NULL);

    return NULL;
}

/*
 * 向线程池中添加任务。
 * 添加成功返回 0 ，否则返回 -1 。
 */
int threadpool_add_task(threadpool_t* threadpool, task_function_t* func, void* args) {
    if (threadpool == NULL || func == NULL) {
        log_error("arguments invalid.");
        return -1;
    }

    /* 加锁 */
    pthread_mutex_lock(&(threadpool->lock_mutex));

    /* 如果任务队列中任务已满，则阻塞在条件 nfull_cond 上 */
    while ((threadpool->task_num == threadpool->task_queue_size) && (!threadpool->shutdown)) {
        pthread_cond_wait(&(threadpool->nfull_cond), &(threadpool->lock_mutex));
    }

    /* 如果线程池已经关闭 */
    if (threadpool->shutdown) {
        pthread_mutex_unlock(&(threadpool->lock_mutex));
        log_error("threadpool already shutdown.");
        return -1;
    }

    /* 添加任务到任务队列尾部 */
    threadpool->task_queue[threadpool->task_queue_rear].func = func;
    threadpool->task_queue[threadpool->task_queue_rear].args = args;
    threadpool->task_queue_rear = (threadpool->task_queue_rear + 1) % threadpool->task_queue_size;
    threadpool->task_num ++ ;

    /* 通知一个阻塞在 nempty_cond 上的线程并解锁互斥锁 */
    pthread_cond_signal(&(threadpool->nempty_cond));
    pthread_mutex_unlock(&(threadpool->lock_mutex));

    return 0;
}

/*
 * 释放线程池的资源。
 * 成功返回 0 ，否则返回 -1 。
 */
static int threadpool_free(threadpool_t* threadpool) {
    if (threadpool == NULL) {
        return -1;
    }

    /* 释放线程 tid 数组空间 */
    if (threadpool->threads) {
        free(threadpool->threads);
        threadpool->threads = NULL;

        /* 由于初始化互斥量和条件变量在 threads 之前，所以此处一定已经初始化成功 */
        pthread_mutex_lock(&(threadpool->lock_mutex));
        pthread_mutex_destroy(&(threadpool->lock_mutex));
        pthread_cond_destroy(&(threadpool->nfull_cond));
        pthread_cond_destroy(&(threadpool->nempty_cond));
    }

    /* 释放任务队列空间 */
    if (threadpool->task_queue) {
        free(threadpool->task_queue);
        threadpool->task_queue = NULL;
    }

    /* 释放线程池空间 */
    free(threadpool);
    threadpool = NULL;

    return 0;
}

/*
 * 销毁线程池。
 * 成功返回 0 ，否则返回 -1 。
 */
int threadpool_destroy(threadpool_t* threadpool) {
    int i;

    do {
        if (threadpool == NULL) {
            log_error("arguments invalid.");
            break;
        }

        /* 取得互斥锁资源 */
        if (pthread_mutex_lock(&(threadpool->lock_mutex)) != 0) {
            log_error("lock failed.");
            break;
        }

        /* 如果线程池已经关闭，则直接返回 */
        if (threadpool->shutdown) {
            log_error("already shutting down.");
            break;
        }

        /* 将线程池设置为关闭 */
        threadpool->shutdown = 1;

        /* 广播通知线程终止并解锁 */
        if ((pthread_cond_broadcast(&(threadpool->nempty_cond)) != 0) ||
            (pthread_mutex_unlock(&(threadpool->lock_mutex)) != 0)) {
            log_error("threadpool unlock failed.");
            break;
        }

        /* 回收线程池中的所有线程 */
        for (i = 0; i < threadpool->thread_running; ++ i) {
            if (pthread_join(threadpool->threads[i], NULL) != 0) {
                log_error("thread join failed.");
            }
        }

        return threadpool_free(threadpool);

    } while (0);

    return -1;
}