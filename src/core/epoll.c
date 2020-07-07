/**
 * @author ttoobne
 * @date 2020/6/5
 */

#include "epoll.h"

#include "log.h"

#include <stddef.h>
#include <stdlib.h>

/*
 * 创建 epoll 文件描述符并初始化 epoll_t 类型。
 * 参数 flags 同 epoll_create1 的参数。
 * 创建成功则返回 epoll_t 类型指针，失败则返回 NULL 。
 */
epoll_t* epoll_create_fd(int flags) {
    epoll_t* epoll = NULL;

    do {
        if ((epoll = (epoll_t*)malloc(sizeof(epoll_t))) == NULL) {
            log_error("epoll malloc failed.");
            break;
        }

        /* 创建 epoll 文件描述符 */
        if ((epoll->epollfd = epoll_create1(flags)) < 0) {
            log_error("epoll fd create failed.");
            break;
        }

        if ((epoll->events = (epoll_event*)malloc(sizeof(epoll_event) * MAX_EVENTS)) == NULL) {
            log_error("events malloc failed.");
            break;
        }

        return epoll;

    } while (0);

    if (epoll_free(epoll) != 0) {
        log_error("epoll free failed.");
    }
    return NULL;
}

/*
 * 添加对 fd 的监听事件。
 */
int epoll_add_fd(epoll_t* epoll, int fd, epoll_event* event) {

    if (epoll == NULL) {
        log_error("epoll is null.");
        return -1;
    }

    /* 添加监听事件 */
    if (epoll_ctl(epoll->epollfd, EPOLL_CTL_ADD, fd, event) != 0) {
        log_error("epoll add fd failed.");
        return -1;
    }

    return 0;
}

/*
 * 修改对于 fd 的监听事件。
 */
int epoll_modify_fd(epoll_t* epoll, int fd, epoll_event* event) {

    if (epoll == NULL) {
        log_error("epoll is null.");
        return -1;
    }

    /* 修改监听事件 */
    if (epoll_ctl(epoll->epollfd, EPOLL_CTL_MOD, fd, event) != 0) {
        log_error("epoll mod failed.");
        return -1;
    }

    return 0;
}

/*
 * 删除对 fd 的监听事件。
 */
int epoll_delete_fd(epoll_t* epoll, int fd, epoll_event* event) {
    
    if (epoll == NULL) {
        log_error("epoll is null.");
        return -1;
    }

    /* 删除监听事件 */
    if (epoll_ctl(epoll->epollfd, EPOLL_CTL_DEL, fd, event) != 0) {
        log_error("epoll delete failed.");
        return -1;
    }

    return 0;
}

/*
 * 等待任意监听的事件就绪。
 */
int epoll_wait_event(epoll_t* epoll, int maxevents, int timeout) {
    int n;
    int events_num;

    if (epoll == NULL) {
        log_error("epoll is null.");
        return -1;
    }

    /* 最大事件数与宏 MAX_EVENTS 取较小值 */
    events_num = maxevents < MAX_EVENTS ? maxevents : MAX_EVENTS;
    /* 等待就绪事件 */
    if ((n = epoll_wait(epoll->epollfd, epoll->events, events_num, timeout)) < 0) {
        log_error("epoll wait failed.");
    }

    return n;
}

/*
 * 释放所申请的空间资源。
 */
int epoll_free(epoll_t* epoll) {
    if (epoll != NULL) {
        free(epoll->events);
        epoll->events = NULL;
        free(epoll);
        epoll = NULL;
    }

    return 0;
}