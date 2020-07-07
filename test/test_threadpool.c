/**
 * @author ttoobne
 * @date 2020/6/5
 */

#include "debug.h"
#include "threadpool.h"

#include <stdio.h>
#include <unistd.h>

#define THREAD_NUM 128
#define QUEUE_SIZE 1024

pthread_mutex_t wnum_lock;
int work_num;
int tot_num;

void* working(void* args) {
    usleep(10000);
    pthread_mutex_lock(&wnum_lock);
    work_num ++ ;
    pthread_mutex_unlock(&wnum_lock);
    return NULL;
}

int main() {
    int i;
    threadpool_t* pool = NULL;
    ASSERT((pool = threadpool_create(THREAD_NUM, QUEUE_SIZE)) != NULL, "threadpool create failed.");
    
    work_num = 0;
    tot_num = QUEUE_SIZE << 4;
    for (i = 0; i < tot_num; ++ i) {
        threadpool_add_task(pool, working, NULL);
    }

    while (work_num < tot_num) {
        usleep(100000);
    }

    ASSERT(threadpool_destroy(pool) == 0, "threadpool destory failed.");

	printf("done.\nwork_num: %d\ntot_num: %d\n", work_num, tot_num);

    return 0;
}
