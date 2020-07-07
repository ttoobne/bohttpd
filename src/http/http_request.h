/**
 * @author ttoobne
 * @date 2020/6/22
 */

#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include "config.h"
#include "epoll.h"
#include "http_timer.h"
#include "list.h"

#define REQUEST_OK                  0
#define REQUEST_ERROR               -1
#define REQUEST_AGAIN               -2
#define REQUEST_TIMEOUT             -3

#define HTTP_UNKNOWN                0x0001
#define HTTP_GET                    0x0002
#define HTTP_HEAD                   0x0004
#define HTTP_POST                   0x0008

#define HTTP_PARSE_INVALID_METHOD   10
#define HTTP_PARSE_INVALID_REQUEST  11
#define HTTP_PARSE_INVALID_HEADER   12

#define HTTP_CONTINUE               100
#define HTTP_OK                     200
#define HTTP_MOVED_PERMANENTLY      301
#define HTTP_MOVED_TEMPORARILY      302
#define HTTP_NOT_MODIFIED           304
#define HTTP_BAD_REQUEST            400
#define HTTP_FORBIDDEN              403
#define HTTP_NOT_FOUND              404
#define HTTP_PRECONDITION_FAILED    412
#define HTTP_INTERNAL_SERVER_ERROR  500
#define HTTP_NOT_IMPLEMENTED        501
#define HTTP_VERSION_NOT_SUPPORTED  505

#define BUF_SIZE                    8192

typedef struct http_request_s http_request_t;

typedef int (request_handler_t) (http_request_t* rq);

/* 表征当前的 http 请求事件。*/
struct http_request_s{
    int                 fd;                     /* 已连接描述符 */
    void*               epoll;                  /* epoll_t 结构体指针 */

                                                /* 开始与结束定位指针 */
    void*               request_start;          /* 整个请求行 */
    void*               request_end;
    void*               method_start;           /* 方法 */
    void*               method_end;
    void*               uri_start;              /* uri */
    void*               args_start;             /* 参数 */
    void*               uri_end;

    unsigned            state;                  /* 当前所处状态 */
    unsigned            method;                 /* 解析出的方法 */
    unsigned            have_args;              /* 有无参数 */
    unsigned            http_version_minor:16;  /* http 主版本号 */
    unsigned            http_version_major:16;  /* http 子版本号 */

    list_head_t         headers_list_head;      /* 保存首部字段的链表头 */
    void*               cur_header_name_start;  /* 定位当前处理的首部字段名 */
    void*               cur_header_name_end;
    void*               cur_header_value_start; /* 定位当前处理的首部字段值 */
    void*               cur_header_value_end;

    void*               root;                   /* 当前所在的主目录 */
    void*               defile;                 /* 默认文件 */

    unsigned char       buf[BUF_SIZE];          /* 缓冲区 */
    unsigned char*      bufst;                  /* 当前缓冲区可读的第一个字节下标 */
    unsigned char*      bufed;                  /* 当前缓冲区不可读/可写的第一个字节下标 */

    http_timer_t        timer;                  /* 定时器 */

    msec_t              timeout;                /* 长连接的超时时间 */

    request_handler_t*  handler;                /* 当前应该执行的解析函数指针 */
};

/* 保存单个 http 请求首部字段。*/
typedef struct {
    list_head_t         list_node;              /* 连入链表所需要的额外信息 */
    void*               header_name_start;      /* 请求首部字段名的开始地址 */
    void*               header_name_end;        /* 请求首部字段名的结束地址 */
    void*               header_value_start;     /* 请求首部字段值的开始地址 */
    void*               header_value_end;       /* 请求首部字段值的结束地址 */
} http_header_t;

/* 保存解析完毕要发送响应的 headers 信息。 */
typedef struct {
    unsigned            keep_alive:4;
    unsigned            if_modified:2;
    unsigned            if_unmodified:2;
    unsigned            status:24;              /* 状态码 */
    time_t              rtime;                  /* 请求报文的创建时间 */
    time_t              mtime;                  /* 所请求资源的修改时间 */
} http_headers_out_t;

typedef int http_headers_handler_t (http_request_t*, http_headers_out_t*, char*, char*);

/* 首部字段名映射到处理函数的函数指针 */
typedef struct {
    char*                   name;
    http_headers_handler_t* handler;
} http_headers_in_t;

/*
 * 初始化 http_request_t 结构体，如果成功返回结构体指针，失败则返回 NULL 。
 */
http_request_t* http_request_init(int fd, epoll_t* epoll, config_t* config);

/*
 * 初始化 http_headers_out_t 结构体，如果成功返回结构体指针，失败则返回 NULL 。
 */
http_headers_out_t* http_headers_out_init();

/*
 * 分析首部字段链表中的所有首部字段，并将结果信息保存至 out 指向的结构体。
 */
int http_analyze_headers(http_request_t* rq, http_headers_out_t* out);

/*
 * 销毁 http_request_t 结构体。
 */
int http_request_destroy(http_request_t* rq);

/*
 * 销毁 http_headers_out_t 结构体。
 */
int http_headers_out_destroy(http_headers_out_t* out);

#endif /* _HTTP_REQUEST_H_ */