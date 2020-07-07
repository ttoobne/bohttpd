/**
 * @author ttoobne
 * @date 2020/6/23
 */

#include "http_request.h"

#include "http.h"
#include "http_parse.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

static int http_process_request_line(http_request_t* rq);
static int http_process_request_headers(http_request_t* rq);

static int http_process_date(http_request_t* rq, http_headers_out_t* out, char* st, char* ed);
static int http_process_connection(http_request_t* rq, http_headers_out_t* out, char* st, char* ed);
static int http_process_if_modified_since(http_request_t* rq, http_headers_out_t* out, char* st, char* ed);
static int http_process_if_unmodified_since(http_request_t* rq, http_headers_out_t* out, char* st, char* ed);

/* 首部字段名映射到处理函数的函数指针 */
http_headers_in_t http_headers_in[] = {
    {"Connection", http_process_connection},
    {"Date", http_process_date},
    {"Host", NULL}, /* TODO: Host */
    {"If-Modified-Since", http_process_if_modified_since},
    {"If-Unmodified-Since", http_process_if_unmodified_since},
    {"", NULL}
};

/*
 * 初始化 http_request_t 结构体，如果成功返回结构体指针，失败则返回 NULL 。
 */
http_request_t* http_request_init(int fd, epoll_t* epoll, config_t* config) {
    http_request_t* rq;

    if ((rq = (http_request_t*)malloc(sizeof(http_request_t))) == NULL) {
        log_error("http_request_t malloc failed.");
        return NULL;
    }

    rq->fd = fd;
    rq->epoll = epoll;
    rq->state = 0;
    rq->have_args = 0;
    rq->bufst = &(rq->buf[0]);
    rq->bufed = &(rq->buf[0]);
    rq->handler = http_process_request_line;    /* 初始时解析请求行 */
    rq->timer.timer_set = 0;
    rq->timer.handler = http_close_connection;  /* 定时器超时回调函数 */
    if (config) {
        rq->root = config->root;
        rq->defile = config->defile;
        rq->timeout = config->timeout;
    }
    
    init_list_head(&(rq->headers_list_head));   /* 初始化链表 */

    return rq;
}

/*
 * 初始化 http_headers_out_t 结构体，如果成功返回结构体指针，失败则返回 NULL 。
 */
http_headers_out_t* http_headers_out_init() {
    http_headers_out_t* out;

    if ((out = (http_headers_out_t*)malloc(sizeof(http_headers_out_t))) == NULL) {
        log_error("http_headers_out_t malloc failed.");
        return NULL;
    }

    out->keep_alive = 0;
    out->if_modified = 0;
    out->if_unmodified = 0;
    out->status = 0;

    return out;
}

/*
 * 分析首部字段链表中的所有首部字段，并将结果信息保存至 out 指向的结构体。
 */
int http_analyze_headers(http_request_t* rq, http_headers_out_t* out) {
    http_header_t* pos;
    list_head_t* head;
    list_head_t* now;
    list_head_t* next;
    http_headers_in_t* in;
    int len;

    /* 获取链表头 */
    head = &(rq->headers_list_head);

    /* 遍历链表中的每一个节点 */
    list_for_each_entry(pos, head, list_node) {
        /* 对于每一个节点循环判断与每一个字段名是否匹配 */
        len = pos->header_name_end - pos->header_name_start + 1;
        for (in = http_headers_in; strlen(in->name) > 0; ++ in) {
            /* 如果匹配，则执行相应处理函数 */
            if (!strncmp(in->name, pos->header_name_start, len)) {
                if (in->handler != NULL) {
                    in->handler(rq, out, pos->header_value_start, pos->header_value_end);
                }

                break;
            }
        }
    }

    /* 销毁链表中的所有节点 */
    now = head->next;
    while (now != head) {
        next = now->next;
        pos = list_entry(now, http_header_t, list_node);

        list_del(now);
        free(pos);
        pos = NULL;

        now = next;
    }

    return 0;
}

/*
 * 销毁 http_request_t 结构体。
 */
int http_request_destroy(http_request_t* rq) {
    if (rq) {
        free(rq);
    }

    return 0;
}

/*
 * 销毁 http_headers_out_t 结构体。
 */
int http_headers_out_destroy(http_headers_out_t* out) {
    if (out) {
        free(out);
    }

    return 0;
}

/*
 * 解析请求行。
 */
static int http_process_request_line(http_request_t* rq) {
    int ret;

    if (rq->timer.timeout) {
        return REQUEST_TIMEOUT;
    }

    if ((ret = http_parse_request_line(rq)) == REQUEST_OK) {
        /* 当返回 REQUEST_OK 时，请求行已解析完毕，继续解析首部字段 */
        rq->handler = http_process_request_headers;
        return http_process_request_headers(rq);
    }
    
    return ret;
}

/*
 * 解析首部字段。
 */
static int http_process_request_headers(http_request_t* rq) {
    if (rq->timer.timeout) {
        return REQUEST_TIMEOUT;
    }

    return http_parse_request_headers(rq);
}

static int http_process_connection(http_request_t* rq, http_headers_out_t* out, char* st, char* ed) {
    if (!strncasecmp(st, "keep-alive", 10)) {
        out->keep_alive = 1;
    }

    return REQUEST_OK;
}

static int http_process_date(http_request_t* rq, http_headers_out_t* out, char* st, char* ed) {
    struct tm time;

    if (!strptime(st, "%a, %d %b %Y %H:%M:%S GMT", &time)) {
        log_error("date error.");
        return REQUEST_ERROR;
    }

    out->rtime = mktime(&time);

    return HTTP_OK;    
}

static int http_process_if_modified_since(http_request_t* rq, http_headers_out_t* out, char* st, char* ed) {
    struct tm time;
    time_t ims;

    if (!strptime(st, "%a, %d %b %Y %H:%M:%S GMT", &time)) {
        log_error("date error.");
        return REQUEST_ERROR;
    }

    ims = mktime(&time);

    if (ims < out->mtime) {
        out->if_modified = 1;   /* 返回 200 */
        out->status = HTTP_OK;
    } else {
        out->if_modified = 2;   /* 返回 304 */
        out->status = HTTP_NOT_MODIFIED;
    }

    return HTTP_OK;
}

static int http_process_if_unmodified_since(http_request_t* rq, http_headers_out_t* out, char* st, char* ed) {
    struct tm time;
    time_t iums;

    if (strptime(st, "%a, %d %b %Y %H:%M:%S GMT", &time)) {
        log_error("date error.");
        return REQUEST_ERROR;
    }

    iums = mktime(&time);

    if (iums > out->mtime) {
        out->if_unmodified = 1; /* 返回 412 */
        out->status = HTTP_PRECONDITION_FAILED;
    }

    return HTTP_OK;
}
