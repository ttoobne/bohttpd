/**
 * @author ttoobne
 * @date 2020/6/24
 */

#ifndef _HTTP_PARSE_H_
#define _HTTP_PARSE_H_

#include <stdint.h>

#define str_method_cmp(m, c0, c1, c2, c3) \
     *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

#define LF      (unsigned char) '\n'
#define CR      (unsigned char) '\r'
#define CRLF    "\r\n"

/*
 * 解析请求行。
 */
int http_parse_request_line(void* http_request);

/*
 * 解析首部字段。
 */
int http_parse_request_headers(void* http_request);

#endif /* _HTTP_PARSE_H_ */