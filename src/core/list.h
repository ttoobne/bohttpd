/**
 * @author ttoobne
 * @date 2020/6/23
 */

#ifndef _LIST_H_
#define _LIST_H_

/*
 * 作为结构体中的成员，保存形成链表所需要的额外信息。
 */
typedef struct list_head_s{
    struct list_head_s* prev;
    struct list_head_s* next;
} list_head_t;

/*
 * 初始化链表头。
 */
void init_list_head(list_head_t* lh_ptr);

/*
 * 添加节点。
 */
void __list_add(list_head_t* new, list_head_t* prev, list_head_t* next);

/*
 * 删除节点。
 */
void __list_del(list_head_t* prev, list_head_t* next);

/*
 * 在链表头部添加节点。
 */
void list_add(list_head_t* new, list_head_t* head);

/*
 * 在链表尾部添加节点。
 */
void list_add_tail(list_head_t* new, list_head_t* head);

/*
 * 删除 entry 节点。
 */
void list_del(list_head_t* entry);

/*
 * 判断链表是否为空。
 */
int list_empty(list_head_t* head);

/*
 * 销毁链表中的所有节点
 */
void destroy_list(list_head_t* head);

/*
 * 获取当前成员所在结构体的起始地址。
 */
#define list_entry(ptr, type, member) \
    ((type*)((char*)(ptr) - (size_t)(&((type*)0)->member)))

/*
 * 从前向后遍历链表。
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/*
 * 从后向前遍历链表。
 */
#define list_for_each_prev(pos, head) \
    for (pos = (head)->prev; pos != (head); pos = pos->prev)

/*
 * 作为所在结构体遍历链表。
 * pos 为遍历结构体的指针， head 为链表头， member 为结构体中 list_head_t 的成员名。
 */
#define list_for_each_entry(pos, head, member)  \
    for (pos = list_entry((head)->next, typeof(*pos), member);  \
        &(pos->member) != (head);                               \
        pos = list_entry(pos->member.next, typeof(*pos), member))

#endif /* _LIST_H_ */