/**
 * @author ttoobne
 * @date 2020/6/3
 */

#include "rio.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n);

/*
 * 更健壮的不带缓冲的读入，从 fd 最多传送 n 字节到 usrbuf 中。
 */
ssize_t rio_readn(int fd, void* usrbuf, size_t* n) {
    size_t nleft = *n;              /* 剩余多少字节未读 */
    ssize_t nread;
    char* bufp = (char*)usrbuf;     /* 读到 usrbuf 的位置 */
    
    while (nleft > 0) {
        if ((nread = read(fd, bufp, nleft)) < 0) {
            if (errno != EINTR) {   /* 当 read 被中断则忽略，否则返回错误 */
                *n = *n - nleft;
                return -1;
            }
        } else if (nread == 0) {    /* 读到 EOF */
            break;
        } else {
            bufp += nread;
            nleft -= nread;
        }
    }
    return (*n - nleft);            /* 返回读了多少字节 */
}

/*
 * 更健壮的不带缓冲的写，从 usrbuf 传送 n 字节到 fd 中。
 */
ssize_t rio_writen(int fd, void* usrbuf, size_t n) {
    size_t nleft = n;       /* 剩余多少字节未写 */
    ssize_t nwrite;       
    char* bufp = usrbuf;    /* 从 usrbuf 写的位置 */

    while (nleft > 0) {
        if ((nwrite = write(fd, bufp, nleft)) <= 0) {
            if (errno != EINTR && errno != EAGAIN) {   /* 当 write 被中断则忽略，否则返回错误 */
                return -1;
            }
        } else {
            bufp += nwrite;
            nleft -= nwrite;
        }
    }
    return n;
}

/*
 * 初始化读缓冲区。
 */
void rio_readinit_buf(rio_t* rp, int fd) {
    rp->rio_fd = fd;
    rp->rio_unread = 0;
    rp->rio_bufptr = rp->rio_buf;
}

/*
 * Linux read 带缓冲的版本，与 read 有相同的语义。
 *  一旦缓冲区非空，就复制最多 n 字节数据到用户缓冲区。
 *  在 rio_readn_buf 中以 rio_read 对应 rio_readn 中的 read ，使得二者有相同的结构。
 *  在 rio_readline_buf 中利用缓冲区以加快原先对 read 的调用。
 */
static ssize_t rio_read(rio_t* rp, char* usrbuf, size_t n) {
    size_t cnt;

    while (rp->rio_unread <= 0) {   /* 当内部缓冲区没有内容可读时 */
        if ((rp->rio_unread = read(rp->rio_fd, rp->rio_buf, sizeof rp->rio_buf)) < 0) {
            if (errno != EINTR) {   /* 当 read 被中断则忽略，否则返回错误 */
                return -1;
            }
        } else if (rp->rio_unread == 0) {
            return 0;               /* 读到 EOF，返回 0 */
        } else {
            rp->rio_bufptr = rp->rio_buf;
        }
    }
    
    cnt = rp->rio_unread < n ? rp->rio_unread : n;  /* 实际读到的字节数 */
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_unread -= cnt;
    return cnt;
}

/*
 * 带缓冲的读入，从内部缓冲区最多传送 n 字节到 usrbuf 中。
 */
ssize_t	rio_readn_buf(rio_t* rp, void* usrbuf, size_t n) {
    size_t nleft = n;       /* 剩余多少字节未读 */
    ssize_t nread;
    char* bufp = usrbuf;    /* 读到 usrbuf 的位置 */

    while (nleft > 0) {
        if ((nread = rio_read(rp, bufp, nleft)) < 0) {
            return -1;
        } else if (nread == 0) {
            return 0;       /* 读到 EOF，返回 0 */
        } else {
            bufp += nread;
            nleft -= nread;
        }
    }
    return (n - nleft);
}

/*
 * 读入一行文本，从内部缓冲区传送一行最多 maxlen 字节到 usrbuf 中。
 */
ssize_t	rio_readline_buf(rio_t* rp, void* usrbuf, size_t maxlen) {
    size_t n, rc;
    char c;
    char* bufp = usrbuf;

    for (n = 1; n <= maxlen; ++ n) {
        if ((rc = rio_read(rp, &c, 1)) == 1) {
            *bufp ++ = c;
            if (c == '\n') {        /* 读到换行符 */ 
                n ++ ;
                break;
            }
        } else if (rc == 0) {       /* 读到 EOF */
            break;
        } else {
            return -1;
        }
    }
    *bufp = '\0';
    return (n - 1);
}