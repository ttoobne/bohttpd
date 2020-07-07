/**
 * @author ttoobne
 * @date 2020/6/29
 */

#include "http.h"

#include "http_request.h"
#include "http_timer.h"
#include "log.h"
#include "rio.h"
#include "utility.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* 文件后缀到完整类型的映射 */
static mime_type_t mimes[] = {
    {".html", "text/html; charset=UTF-8"},
    {".htm", "text/html; charset=UTF-8"},
    {".xhtml", "application/xhtml+xml; charset=UTF-8"},
    {".xht", "application/xhtml+xml; charset=UTF-8"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".png", "image/png"},
    {".css", "text/css"},
    {".xml", "text/xml; charset=UTF-8"},
    {".xsl", "text/xml; charset=UTF-8"},
    {".au", "audio/basic"},
    {".wav", "audio/wav"},
    {".avi", "video/x-msvideo"},
    {".mov", "video/quicktime"},
    {".qt", "video/quicktime"},
    {".mpeg", "video/mpeg"},
    {".mpe", "video/mpeg"},
    {".vrml", "model/vrml"},
    {".wrl", "model/vrml"},
    {".midi", "audio/midi"},
    {".mid", "audio/midi"},
    {".mp3", "audio/mpeg"},
    {".ogg", "application/ogg"},
    {".pac", "application/x-ns-proxy-autoconfig"},
    {NULL, NULL}
};

static unsigned parse_uri(http_request_t* rq, char* filename);
static int serve_headers(http_request_t* rq, http_headers_out_t* out, char* mime_type, off_t length, unsigned errstatus);
static int serve_static(http_request_t* rq, http_headers_out_t* out, char* filename, off_t length);
static int serve_error(http_request_t* rq, unsigned status);
static char* get_shortmsg(unsigned status);
static char* get_mime_type(char* filename);

/*
 * 对已连接描述符的事件进行初始化。
 */
int http_init_connection(int connfd, epoll_t* epoll, config_t* config) {
    http_request_t* rq;
    struct epoll_event epev;

    /* 非阻塞读写 */
    set_nonblocking(connfd);

    if ((rq = http_request_init(connfd, epoll, config)) == NULL) {
        log_error("http_request_t init failed.");
        return -1;
    }

    /* 设置 epoll 监听 connfd 上的读事件，边缘触发 */
    epev.data.ptr = (void*)rq;
    epev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

    epoll_add_fd(epoll, connfd, &epev);

    /* 添加事件到定时器，设置超时回调函数 */
    add_timer((void*)rq, rq->timeout, http_close_connection);

    return 0;
}
/*
 * 创建监听描述符。
 */
int create_listenfd(unsigned short port) {
    int listenfd;
    int optval;
    struct sockaddr_in servaddr;

    /* 创建 socket 描述符 */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_error("create socket failed.");
        return -1;
    }

    /* 设置 SO_REUSEADDR 关闭服务端的 TIME_WAIT */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0) {
        log_error("set listenfd reuse adress error.");
	    return -1;
    }

    /* 设置地址与端口号 */
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons((unsigned short)port);

    /* 绑定地址与端口号 */
    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        log_error("bind error.");
        return -1;
    }

    /* 将描述符设置为监听描述符 */
    if (listen(listenfd, BACKLOG) < 0) {
        log_error("listen error.");
        return -1;
    }

    return listenfd;
}

/*
 * 执行请求。
 */
void* execute_request(void* http_request) {
    http_request_t* rq;
    http_headers_out_t* out;
    struct epoll_event epev;
    char filename[MAXLINE] = {'\0'};
    struct stat statbuf;
    size_t remain;
    ssize_t size;
    int ret;

    rq = (http_request_t*)http_request;

    if ((out = http_headers_out_init()) == NULL) {
        log_error("http_headers_out_t init failed.");
        return NULL;
    }

    delete_timer((void*)rq);

    for ( ;; ) {
        remain = &(rq->buf[BUF_SIZE - 1]) - rq->bufed;

        if (remain <= 0) {
            serve_error(rq, HTTP_BAD_REQUEST);

            goto close;
        }

        size = rio_readn(rq->fd, rq->bufed, &remain);

        /* size 返回 -1 */
        /* 1. 读到 EAGAIN ，所读字节数作为值-结果参数返回 */
        /* 2. 其他 errno ，出错 */

        /* size 返回大于等于 0 */
        /* 3. 读到 EOF 返回不足值 */
        /* 4. 读到 EOF 返回 0 */
        /* 5. 没读到 EOF 返回所读的值，此时缓冲区已满 */
        if (size < 0) {
            if (errno != EAGAIN) {
                log_error("read error.");
                serve_error(rq, HTTP_INTERNAL_SERVER_ERROR);
                
                goto close;
            }
            
            if (remain == 0) {
                break;
            }

            rq->bufed += remain;
        } else if (size == 0) {
            goto close;
        } else {
            rq->bufed += size;
        }

        /* 解析请求头，直到出错或完成 */
        if ((ret = rq->handler(rq)) == REQUEST_AGAIN) {
            if (size > 0 && size < remain) {
                goto close;
            }

            continue;
        } else if (ret != REQUEST_OK) {
            serve_error(rq, HTTP_BAD_REQUEST);
            
            goto close;
        }

        /* TODO: CGI&POST */
        if (rq->method != HTTP_GET && rq->method != HTTP_HEAD) {
            serve_error(rq, HTTP_NOT_IMPLEMENTED);

            goto close;
        }

        /* http 版本号超过 1.1 则不支持 */
        if (rq->http_version_major * 1000 + rq->http_version_minor > 1001) {
            serve_error(rq, HTTP_VERSION_NOT_SUPPORTED);

            goto close;
        }

        memset(filename, 0, sizeof(filename));
        parse_uri(rq, filename);

        /* 没找到改文件，返回 404 */
        if (stat(filename, &statbuf) != 0) {
            serve_error(rq, HTTP_NOT_FOUND);

            goto close;
        }

        /* 权限不够，返回 403 */
        if (!S_ISREG(statbuf.st_mode) || !(statbuf.st_mode & S_IRUSR)) {
            serve_error(rq, HTTP_FORBIDDEN);

            goto close;
        }

        out->keep_alive = 0;
        out->if_modified = 0;
        out->if_unmodified = 0;
        out->status = 0;
        out->mtime = statbuf.st_mtime;

        /* 分析首部字段 */
        if (http_analyze_headers(rq, out) != 0) {
            log_error("analyze headers failed.");
            serve_error(rq, HTTP_INTERNAL_SERVER_ERROR);
            
            goto close;
        }

        if (out->if_modified == 2) {
            serve_error(rq, HTTP_NOT_MODIFIED);

            goto close;
        }

        if (out->if_unmodified) {
            serve_error(rq, HTTP_PRECONDITION_FAILED);

            goto close;
        }

        if (out->status == 0) {
            out->status = HTTP_OK;
        }

        /* 发送静态文件 */
        serve_static(rq, out, filename, statbuf.st_size);

        if (!out->keep_alive) {
            goto close;
        }

        if (size > 0 && size < remain) {
            goto close;
        }

        /* 长连接处理第二个请求， HTTP/1.1 请求不会并行执行 */
        rq->bufst = &(rq->buf[0]);
        rq->bufed = &(rq->buf[0]);
        
    }

    /* 当前读完，但是没有关闭连接（返回 EAGAIN） */
    /* 则需要再次设置 epoll 监听（边缘触发） */
    epev.data.ptr = (void*)rq;
    epev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_modify_fd((epoll_t*)rq->epoll, rq->fd, &epev);

    /* 再次定时 */
    add_timer((void*)rq, rq->timeout, http_close_connection);
    
    http_headers_out_destroy(out);
    
    return NULL;

close:

    /* 关闭连接 */
    http_close_connection(rq);
    http_headers_out_destroy(out);

    return NULL;
}

/*
 * 关闭连接并释放内存。
 */
int http_close_connection(void* http_request) {
    http_request_t* rq;

    rq = (http_request_t*) http_request;
    
    close(rq->fd);

    http_request_destroy(rq);

    log_info("connection closed.");

    return 0;
}

/*
 * 解析 uri 并将文件名保存至 filename 。
 */
static unsigned parse_uri(http_request_t* rq, char* filename) {
    int len;

    if (rq->have_args) {
        /* TODO: CGI */
    } else {
        len = rq->uri_end - rq->uri_start + 1;

        strcpy(filename, rq->root);
        strncat(filename, rq->uri_start, len);

        len = strlen(filename);

        /* 防止非法访问父级目录，所以规定 root 中也不能有此行为 */
        if (strstr(filename, "/../") != NULL || strcmp(&(filename[len - 3]), "/..") == 0) {
            return HTTP_BAD_REQUEST;
        }

        /* 如果是目录，则添加默认文件 */
        if (filename[len - 1] == '/') {
            if (rq->defile) {
                strcat(filename, rq->defile);
            } else {
                strcat(filename, "index.html");
            }
        }

    }

    return 0;
}

/*
 * 发送响应头部。
 */
static int serve_headers(http_request_t* rq, http_headers_out_t* out, char* mime_type, off_t length, unsigned errstatus) {
    char headers[MAXMSG] = { '\0' };
    char buf[MAXLINE] = { '\0' };
    struct tm tm;
    time_t now;
    size_t size;

    if (out == NULL) {
        sprintf(headers, "%s %u %s\r\n", PROTOCOL, errstatus, get_shortmsg(errstatus));

        sprintf(headers, "%sServer: %s\r\n", headers, SERVER_NAME);

        now = time((time_t *)0);
        strftime(buf, MAXLINE, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
        sprintf(headers, "%sDate: %s\r\n", headers, buf);

        sprintf(headers, "%sConnection: close\r\n", headers);

        if (mime_type) {
            sprintf(headers, "%sContent-type: %s\r\n", headers, mime_type);
        }

        if (length >= 0) {
            sprintf(headers, "%sContent-length: %lld\r\n", headers, (long long)length);
        }

        sprintf(headers, "%s\r\n", headers);
    } else {
        sprintf(headers, "%s %u %s\r\n", PROTOCOL, out->status, get_shortmsg(out->status));

        sprintf(headers, "%sServer: %s\r\n", headers, SERVER_NAME);

        now = time((time_t *)0);
        strftime(buf, MAXLINE, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
        sprintf(headers, "%sDate: %s\r\n", headers, buf);

        if (out->keep_alive) {
            sprintf(headers, "%sConnection: keep-alive\r\n", headers);
            sprintf(headers, "%sKeep-Alive: timeout=%lu\r\n", headers, rq->timeout);
        }

        if (mime_type) {
            sprintf(headers, "%sContent-type: %s\r\n", headers, mime_type);
        }

        if (length >= 0) {
            sprintf(headers, "%sContent-length: %lld\r\n", headers, (long long)length);
        }

        if (out->if_modified) {
            localtime_r(&(out->mtime), &tm);
            strftime(buf, MAXLINE, "%a, %d %b %Y %H:%M:%S GMT", &tm);
            sprintf(headers, "%sLast-Modified: %s\r\n", headers, buf);
        }

        sprintf(headers, "%s\r\n", headers);
    }

    size = rio_writen(rq->fd, headers, strlen(headers));

    if (size < 0) {
        log_error("send headers error.");
        return -1;
    }

    return 0;
}

/*
 * 发送静态文件。
 */
static int serve_static(http_request_t* rq, http_headers_out_t* out, char* filename, off_t length) {
    int srcfd;
    char* srcaddr;
    size_t size;

    serve_headers(rq, out, get_mime_type(filename), length, 0);

    if ((srcfd = open(filename, O_RDONLY, 0)) <= 2) {
        log_error("open file error.");
        return -1;
    }

    /* 将文件映射到虚拟地址空间，不用将文件先读到用户态，高效读取文件 */
    if ((srcaddr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, srcfd, 0)) == (void*)-1) {
        log_error("mmap error.");
        close(srcfd);
        return -1;
    }
    
    close(srcfd);

    if ((size = rio_writen(rq->fd, srcaddr, length)) < 0) {
        log_error("write error.");
        munmap(srcaddr, length);
        return -1;
    }

    munmap(srcaddr, length);

    return 0;

}

/*
 * 发送错误信息。
 */
static int serve_error(http_request_t* rq, unsigned status) {
    char body[MAXMSG] = { '\0' };
    int length;

    sprintf(body, "<html><head>");
    sprintf(body, "%s<title>%d %s</title></head>", body, status, get_shortmsg(status));
    sprintf(body, "%s<body bgcolor=\"LightSkyBlue\" align=\"center\">", body);
    sprintf(body, "%s<h1>%d %s</h1><hr>", body, status, get_shortmsg(status));
    sprintf(body, "%s<em>%s</em>", body, SERVER_NAME);
    sprintf(body, "%s</body></html>", body);

    length = strlen(body);

    serve_headers(rq, NULL, "text/html; charset=UTF-8", length, status);

    if (rio_writen(rq->fd, body, length) < 0) {
        log_error("write error.");
        return -1;
    }

    return 0;
}

/*
 * 通过状态码获取短消息。
 */
static char* get_shortmsg(unsigned status) {
    switch (status) {
    case HTTP_CONTINUE:
        return "Continue";

    case HTTP_OK:
        return "OK";

    case HTTP_MOVED_PERMANENTLY:
        return "Moved Permanently";

    case HTTP_MOVED_TEMPORARILY:
        return "Found";
        
    case HTTP_NOT_MODIFIED:
        return "Not Modified";
        
    case HTTP_BAD_REQUEST:
        return "Bad Request";
        
    case HTTP_FORBIDDEN:
        return "Forbidden";
        
    case HTTP_NOT_FOUND:
        return "Not Found";
    
    case HTTP_PRECONDITION_FAILED:
        return "Precondition Failed";
        
    case HTTP_INTERNAL_SERVER_ERROR:
        return "Internal Server Error";

    case HTTP_NOT_IMPLEMENTED:
        return "Not Implemented";

    case HTTP_VERSION_NOT_SUPPORTED:
        return "HTTP Version not supported";
    
    default:
        break;
    }

    return "";
}

/*
 * 通过文件名获取文件类型。
 */
static char* get_mime_type(char* filename) {
    char* suffix;
    mime_type_t* it;

    suffix = strrchr(filename, '.');

    if (suffix == NULL) {
        return "text/plain; charset=UTF-8";
    }

    for (it = mimes; it->suffix != NULL; ++ it) {
        if (strcmp(it->suffix, suffix) == 0) {
            return it->type;
        }
    }

    return "text/plain; charset=UTF-8";
}