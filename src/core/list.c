/**
 * @author ttoobne
 * @date 2020/6/23
 */

#include "list.h"

/*
 * 初始化链表头。
 */
void init_list_head(list_head_t* lh_ptr) {
    lh_ptr->next = lh_ptr;
    lh_ptr->prev = lh_ptr;

    return ;
}

/*
 * 添加节点。
 */
void __list_add(list_head_t* new, list_head_t* prev, list_head_t* next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;

    return ;
}

/*
 * 删除节点。
 */
void __list_del(list_head_t* prev, list_head_t* next) {
    next->prev = prev;
    prev->next = next;

    return ;
}

/*
 * 在链表头部添加节点。
 */
void list_add(list_head_t* new, list_head_t* head) {
    __list_add(new, head->prev, head);

    return ;
}

/*
 * 在链表尾部添加节点。
 */
void list_add_tail(list_head_t* new, list_head_t* head) {
    __list_add(new, head->prev, head);

    return ;
}

/*
 * 删除 entry 节点。
 */
void list_del(list_head_t* entry) {
    __list_del(entry->prev, entry->next);

    entry->next = (void*) 0;
    entry->prev = (void*) 0;

    return ;
}

/*
 * 判断链表是否为空。
 */
int list_empty(list_head_t* head) {
    return (head->next == head) && (head->prev == head);
}