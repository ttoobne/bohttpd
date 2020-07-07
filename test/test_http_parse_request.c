/**
 * @author ttoobne
 * @date 2020/6/29
 */

#include "debug.h"
#include "http_parse.h"
#include "http_request.h"
#include "list.h"

#include <time.h>

void print_info(char* pr, char* st, char* ed) {
    printf("%s\n%.*s\n\n", pr, ed - st + 1, st);
}

int main() {
    http_request_t* rq;
    http_header_t* pos;
    list_head_t* head;
    list_head_t* now;
    list_head_t* next;
    http_headers_out_t* out;
    struct tm time;
    time_t mtime;
    int len;
    int ret;

    if ((rq = http_request_init(0, 1)) == NULL) {
        ERROR("http_request_init failed.");
        return 1;
    }

    if ((out = http_headers_out_init(0)) == NULL) {
        ERROR("http_headers_out_init failed.");
        return 1;
    }

    sprintf(rq->buf, "GET /index.html HTTP/1.1\r\n");
    sprintf(rq->buf, "%sHost: ttoobne.com\r\n", rq->buf);
    sprintf(rq->buf, "%sConnection: keep-alive\r\n", rq->buf);
    sprintf(rq->buf, "%sCache-Control: max-age=0\r\n", rq->buf);
    sprintf(rq->buf, "%sUpgrade-Insecure-Requests: 1\r\n", rq->buf);
    sprintf(rq->buf, "%sUser-Agent: Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.116 Mobile Safari/537.36\r\n", rq->buf);
    sprintf(rq->buf, "%sAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n", rq->buf);
    sprintf(rq->buf, "%sAccept-Encoding: gzip, deflate\r\n", rq->buf);
    sprintf(rq->buf, "%sAccept-Language: zh-CN,zh;q=0.9\r\n", rq->buf);
    sprintf(rq->buf, "%sIf-None-Match: \"5ed21299-1c6e\"\r\n", rq->buf);
    len = sprintf(rq->buf, "%sIf-Modified-Since: Sat, 30 May 2020 08:00:25 GMT\r\n\r\n", rq->buf);

    //DBG("%s", rq->buf);

    rq->bufed += len;

    //DBG("bufst: %d\n", rq->bufst);
    //DBG("bufed: %d\n", rq->bufed);
    //DBG("bufend if end = '\\n': %d\n", rq->buf[rq->bufed - 1] == '\n');

    //DBG("before while\n");

    while((ret = rq->handler(rq)) != REQUEST_OK) {
        if (ret != REQUEST_AGAIN) {
            DBG("error return value: %d\n", ret);
            DBG("state: %d\n", rq->state);
            return 1;
        }
    }

    //DBG("after while\n");
    print_info("whole request line:", rq->request_start, rq->request_end);
    print_info("method:", rq->method_start, rq->method_end);
    print_info("uri:", rq->uri_start, rq->uri_end);
    printf("version:\nHTTP/%d.%d\n\n", rq->http_version_major, rq->http_version_minor);

    head = &(rq->headers_list_head);

    list_for_each_entry(pos, head, list_node) {
        print_info("name:", pos->header_name_start, pos->header_name_end);
        print_info("value:", pos->header_value_start, pos->header_value_end);
    }

    strptime("Sat, 30 May 2020 08:00:30 GMT", "%a, %d %b %Y %H:%M:%S GMT", &time);
    mtime = mktime(&time);
    out->mtime = mtime;

    http_analyze_headers(rq, out);

    printf("out: keep_alive: %d\n", out->keep_alive);
    printf("out: if_modified: %d\n", out->if_modified);
    printf("out: if_unmodified: %d\n", out->if_unmodified);

    http_request_destroy(rq);
    http_headers_out_destroy(out);

    return 0;
}