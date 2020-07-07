/**
 * @author ttoobne
 * @date 2020/6/3
 */

/*
 * 部分定义和实现参考自CSAPP第10章内容。
 */

#ifndef _RIO_H_
#define _RIO_H_

#include <sys/types.h>

#define RIO_BUFSIZE 8192

typedef struct {
    int         rio_fd;                 /* 内部缓冲区的文件描述符 */
    ssize_t     rio_unread;             /* 内部缓冲区未读字节 */
    char*       rio_bufptr;             /* 内部缓冲区中下一个未读字节 */
    char        rio_buf[RIO_BUFSIZE];   /* 内部缓冲区 */
} rio_t;

ssize_t rio_readn(int fd, void* usrbuf, size_t* n);     /* 不带内部缓冲区的读 */
ssize_t rio_writen(int fd, void* usrbuf, size_t n);     /* 不带内部缓冲区的写 */
void rio_readinit_buf(rio_t* rp, int fd);               /* 内部缓冲区初始化 */
/* 注意带内部缓冲的读和不带内部缓冲的读不能混合使用 */
ssize_t	rio_readn_buf(rio_t* rp, void* usrbuf, size_t n);           /* 带内部缓冲区的 readn */
ssize_t	rio_readline_buf(rio_t* rp, void* usrbuf, size_t maxlen);   /* 带内部缓冲区的 readline */

#endif /* _RIO_H_ */
