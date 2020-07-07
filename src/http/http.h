/**
 * @author ttoobne
 * @date 2020/6/23
 */

#ifndef _HTTP_H_
#define _HTTP_H_

#include "config.h"
#include "epoll.h"

#define PROTOCOL    "HTTP/1.1"
#define SERVER_NAME "bohttpd/1.0"

#define BACKLOG     1024
#define MAXLINE     512
#define MAXMSG      4096

/* 文件后缀到完整类型的映射 */
typedef struct {
    char* suffix;   /* 文件后缀 */
    char* type;     /* 文件类型 */
} mime_type_t;

/*
 * 对已连接描述符的事件进行初始化。
 */
int http_init_connection(int connfd, epoll_t* epoll, config_t* config);

/*
 * 创建监听描述符。
 */
int create_listenfd(unsigned short port);

/*
 * 执行请求。
 */
void* execute_request(void* http_request);

/*
 * 关闭连接并释放内存。
 */
int http_close_connection(void* http_request);

#endif /* _HTTP_H_ */