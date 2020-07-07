/**
 * @author ttoobne
 * @date 2020/6/29
 */

#include "debug.h"
#include "list.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int val;
    list_head_t list_node;
} node_t;

int main() {
    list_head_t lh;
    node_t* pos;
    list_head_t* head;
    list_head_t* now;
    list_head_t* next;
    int i;

    init_list_head(&lh);

    for (i = 0; i < 32; ++ i) {
        pos = (node_t*)malloc(sizeof(node_t));
        pos->val = i;
        list_add(&(pos->list_node), &lh);
    }

    i = 0;
    head = &lh;
    list_for_each_entry(pos, head, list_node) {
        ASSERT(pos->val == i ++, "error.\n");
    }

    now = head->next;
    while (now != head) {
        next = now->next;
        pos = list_entry(now, node_t, list_node);
        list_del(now);
        free(pos);
        pos = NULL;
        now = next;
    }

    list_for_each_entry(pos, head, list_node) {
        printf("%d\n", pos->val);
    }
    
    DBG("debug done.\n");

    return 0;
}