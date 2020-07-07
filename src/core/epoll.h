/**
 * @author ttoobne
 * @date 2020/6/5
 */

#ifndef _EPOLL_H_
#define _EPOLL_H_

#include <sys/epoll.h>

/*
 * 使用 epoll_t 类型简化 epoll 的使用。
 */

#define MAX_EVENTS 1024
typedef struct epoll_event epoll_event;

typedef struct {
    int             epollfd;   /* epoll 对应的文件描述符 */
    epoll_event*    events;    /* epoll_wait_event 返回就绪的事件数组 */
} epoll_t;

/*
 * 创建 epoll 文件描述符并初始化 epoll_t 类型。
 * 参数 flags 同 epoll_create1 的参数。
 * 创建成功则返回 epoll_t 类型指针，失败则返回 NULL 。
 */
epoll_t* epoll_create_fd(int flags);

/*
 * 添加对 fd 的监听事件。
 */
int epoll_add_fd(epoll_t* epoll, int fd, epoll_event* event);

/*
 * 修改对于 fd 的监听事件。
 */
int epoll_modify_fd(epoll_t* epoll, int fd, epoll_event* event);

/*
 * 删除对 fd 的监听事件。
 */
int epoll_delete_fd(epoll_t* epoll, int fd, epoll_event* event);

/*
 * 等待任意监听的事件就绪。
 */
int epoll_wait_event(epoll_t* epoll, int maxevents, int timeout);

/*
 * 释放所申请的空间资源。
 */
int epoll_free(epoll_t* epoll);

#endif /* _EPOLL_H_ */
