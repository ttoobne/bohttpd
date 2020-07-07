/**
 * @author ttoobne
 * @date 2020/7/2
 */

#include "bohttpd.h"

#include "config.h"
#include "epoll.h"
#include "http.h"
#include "http_request.h"
#include "http_timer.h"
#include "log.h"
#include "threadpool.h"
#include "utility.h"

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* 设置长参数选项 */
static const struct option long_options[] = {
    {"help", no_argument, NULL, '?'},
    {"config", required_argument, NULL, 'c'},
    {"version", no_argument, NULL, 'V'},
    {"testconf", no_argument, NULL, 't'},
    {NULL, 0, NULL, 0}
};

static void usage();

/*
 * bohttpd 主函数。
 */
int main(int argc, char* argv[]) {
    config_t*           config;
    char*               conf_path;
    struct sockaddr_in  cliaddr;
    socklen_t           cliadrlen;
    epoll_t*            epoll;
    struct epoll_event  epev;
    http_request_t*     event;
    threadpool_t*       threadpool;
    msec_t              timeout;
    int                 opt;
    int                 options_index;
    int                 listenfd;
    int                 connfd;
    int                 evnum;

    /* 配置文件默认路径 */
    conf_path = CONF_PATH;

    /* 解析参数 */
    while ((opt = getopt_long(argc, argv, "c:Vt?h", long_options, &options_index)) != EOF) {
        switch (opt) {
            case  0 : break;
            case 'c':
                conf_path = optarg;
                break;
            case 'V':
                fprintf(stderr, BOHTTPD_VER"\n");
                return 1;
            case 't':
                parse_configuration(conf_path);
                return 1;
            case ':':
            case '?':
            case 'h':
                usage();
                return 1;
            default:
                usage();
                return 1;
        }
    }

    /* 解析配置文件 */
    if ((config = parse_configuration(conf_path)) == NULL) {
        log_error("parse configuration failed.");
        return 1;
    }

    log_info("configuration file parsing is complete.");

    /* 初始化定时器 */
    if (init_timer() != 0) {
        log_error("init timer failed.");
        return 1;
    }

    log_info("timer initialization is complete.");

    /* 创建 epoll 文件描述符 */
    if ((epoll = epoll_create_fd(0)) == NULL) {
        log_error("create epoll fd failed.");
        return 1;
    }

    /* 创建监听描述符 */
    if ((listenfd = create_listenfd(config->port)) < 0) {
        log_error("create listenfd failed.");
        return 1;
    }

    /* 将监听描述符设置为非阻塞 */
    set_nonblocking(listenfd);

    /* 为了将 listenfd 放入 epoll 中，需要额外为 listenfd 初始化一个事件结构体  */
    if ((event = http_request_init(listenfd, epoll, NULL)) == NULL) {
        log_error("event_t init failed.");
        return 1;
    }

    /* epoll 监听 listenfd 上的 accept 事件，边缘触发 */
    epev.data.ptr = (void*)event;
    epev.events = EPOLLIN | EPOLLET;
    epoll_add_fd(epoll, listenfd, &epev);

    /* 创建线程池，大小从配置文件读取 */
    if ((threadpool = threadpool_create(config->threadpool, config->taskqueue)) == NULL) {
        log_error("thread poll create failed.");
        return 1;
    }

    cliadrlen = sizeof(cliaddr);    /* 必须初始化 */

    log_info("thread pool initialization is complete.");

    log_info("Bohttpd goes to work.");

    /* 主循环 */
    for ( ;; ) {
        timeout = find_timer();

        /* 根据超时时间最接近的事件确定 epoll wait 的阻塞时间 */
        if ((evnum = epoll_wait_event(epoll, MAX_EVENTS, timeout)) < 0) {
            log_error("epoll wait failed.");
            return 1;
        }

        /* 此时一定有超时事件，需要执行回调函数 */
        expire_timers();

        while(evnum -- ) {
            event = (http_request_t*)(epoll->events[evnum].data.ptr);
            if (event->fd == listenfd) {
                /* 边缘触发，所以必须处理完所有的连接请求 */
                for ( ;; ) {
                    //log_debug("ready to accept");
                    if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &cliadrlen)) < 0) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            log_error("accept error.");
                        }

                        break;
                    }

                    log_info("new connection arrive. client<%s:%u>", 
                            inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
                    
                    /* 初始化已连接描述符，加入定时器、epoll 监听可读事件 */
                    http_init_connection(connfd, epoll, config);
                }
                
            } else {
                if ((epoll->events[evnum].events & EPOLLERR) || 
                    (epoll->events[evnum].events & EPOLLHUP) || /* 对端关闭连接 */
                    !(epoll->events[evnum].events & EPOLLIN)) {

                    close(connfd);
                    continue;
                }

                event = epoll->events[evnum].data.ptr;

                /* 将执行任务添加至工作队列等待线程执行 */
                threadpool_add_task(threadpool, execute_request, (void*)event);

                //execute_request(event);
            }
        }
    }

    config_destroy(config);

    epoll_free(epoll);

    threadpool_destroy(threadpool);

    return 0;
}

/*
 * Usage
 */
static void usage() {
    fprintf(stderr,
	"Usage: bohttpd [option]... \n"
	"  -?,-h,--help                 this help.\n"
	"  -c,--config <filename>       set configuration file. (default: \"./bohttpd.conf\")\n"
	"  -V,--version                 show version and exit.\n"
    "  -t,--testconf                test configuration and exit.\n"
	);
}