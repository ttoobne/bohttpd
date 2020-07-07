/**
 * @author ttoobne
 * @date 2020/7/3
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define NAME_MAX        256             /* 文件名最大长度 */
#define CONFBUF_SIZE    1024            /* 按行读取缓冲区大小 */
#define USHORT_MAX      65535
#define INT_MAX         2147483647

#define THREADPOOL_DEF  64              /* 线程池大小默认值 */
#define TASKQUEUE_DEF   32              /* 任务队列大小默认值 */
#define ROOT_DEF        "./html/"       /* 根目录默认值 */
#define DEFILE_DEF      "index.html"    /* 默认文件默认值 */
#define TIMEOUT_DEF     1000            /* 长连接超时时间默认值 */
#define PORT_DEF        80              /* 端口号默认值 */

typedef struct {
    int             threadpool;         /* 线程池大小 */
    int             taskqueue;          /* 任务队列大小 */
    char            root[NAME_MAX];     /* 根目录 */
    char            defile[NAME_MAX];   /* 默认文件名 */
    unsigned long   timeout;            /* 长连接超时时间 */
    unsigned short  port;               /* 端口号 */
} config_t;

/*
 * 解析配置文件，参数为文件名，返回 config_t 结构体指针。
 */
config_t* parse_configuration(char* filename);

int config_destroy(config_t* config);

#endif /* _CONFIG_H_ */