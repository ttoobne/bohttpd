/**
 * @author ttoobne
 * @date 2020/6/13
 */

#include "debug.h"
#include "log.h"
#include "rbtree.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM 1000000
#define MAX 100000000

typedef struct {
    rbtree_node_t timer_node;
} timer_ty;

typedef struct {
    char buf[4096];
    timer_ty timer;
} request;

struct node {
    int op;
    rbtree_key_t key;
    int deleptr;
    request* rqptr;
} no[] = {
    {1, 578078410, -1},
    {2, 578078410, 0},
    {1, 578079413, -1},
    {2, 578079413, 2},
    {1, 578079413, -1},
    {1, 578079413, -1},
    {1, 578079413, -1},
    {1, 578079413, -1},
    {1, 578079413, -1},
    {1, 578079413, -1},
    {2, 578079413, 8},
    {2, 578079413, 7},  // core dump???
    {2, 578079413, 4},
    {2, 578079413, 5},
    {2, 578079413, 6},
    {2, 578079413, 9},
};

int main() {
    rbtree_t rbtree;
    rbtree_node_t nil;
    int error = 0;

    srand((unsigned)time(NULL));
    
    rbtree_init(&rbtree, &nil, rbtree_insert_timer);

    for (int i = 0; i < 16; ++ i) {
        if (no[i].op == 1) {
            request* rq = (request*)malloc(sizeof(request));
            rq->timer.timer_node.key = no[i].key;
            no[i].rqptr = rq;
            rbtree_insert(&rbtree, &(rq->timer.timer_node));
            log_debug("insert:%d", rq->timer.timer_node.key);
        } else {
            rbtree_node_t* node = &(no[no[i].deleptr].rqptr->timer.timer_node);
            rbtree_delete(&rbtree, node);
            log_debug("delete:%d", no[no[i].deleptr].rqptr->timer.timer_node.key);
            free(no[no[i].deleptr].rqptr);
        }
        
    }

    ASSERT(rbtree.root == rbtree.nil, "rbtree not null!");

    rbtree_node_t* it = (rbtree_node_t*)malloc(sizeof(rbtree_node_t) * NUM);
    for (int i = 0; i < NUM; ++ i) {
       rbtree_node_t* node = ((rbtree_node_t*)it + i);
       node->key = rand() % MAX + 1;
       rbtree_insert(&rbtree, node);
    }

    DBG("insert complete\n");
    
    rbtree_key_t pre = 0;
    for (int i = 0; i < NUM; ++ i) {
       rbtree_node_t* node = rbtree_get_min(&rbtree);
       // DBG("pre:%u now_min:%u\n", pre, node->key);
       if (node->key < pre) {
           error = 1;
           break;
       }
       pre = node->key;
       rbtree_delete(&rbtree, node);
    }

    DBG("delete complete\n");


    if (error) {
        printf("error!\n");
    } else {
        printf("ok!\n");
    }

    free(it);
    it = NULL;

    return 0;
}