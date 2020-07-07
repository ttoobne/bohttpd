/**
 * @author ttoobne
 * @date 2020/6/26
 */

#include "http_parse.h"

#include "http_request.h"
#include "list.h"

#include <stdlib.h>

/*
 * 解析请求行。
 */
int http_parse_request_line(void* http_request) {
    http_request_t* rq;
    unsigned char   ch;
    unsigned char*  p;
    unsigned char*  m;

    rq = (http_request_t*)http_request;

    /* 维护一个状态机来解析 http 请求行 */
    /* 格式：<method> <uri> <version> CRLF */
    enum {
        start = 0,
        method,
        spaces_before_uri,
        after_slash_in_uri,
        spaces_after_uri,
        http_version_H,
        http_version_HT,
        http_version_HTT,
        http_version_HTTP,
        http_version_first_major_digit,
        http_version_major_digit,
        http_version_first_minor_digit,
        http_version_minor_digit,
        spaces_after_digit,
        almost_done
    } state;

    state = rq->state;

    //DBG("ready to parse.\n");

    for (p = rq->bufst; p != rq->bufed; ++ p) {
        ch = *p;

        switch (state) {
        
        /* 开始解析 */
        case start:

            //DBG("start.\n");

            if (ch == CR || ch == LF) {
                break;
            }

            rq->request_start = p;
            rq->method_start = p;

            if (ch < 'A' || ch > 'Z') {
                return HTTP_PARSE_INVALID_METHOD;
            }

            state = method;
            break;  /* case start */

        /* http 请求方法 */
        case method:

            //DBG("method.\n");

            if (ch == ' ') {
                rq->method_end = p - 1;
                m = rq->method_start;

                switch (p - m) { /* method_end - method_start + 1 */
                case 3:
                    /* GET 方法 */
                    if (str_method_cmp(m, 'G', 'E', 'T', ' ')) {
                        rq->method = HTTP_GET;
                        break;
                    }

                    break;
                
                case 4:
                    /* HEAD 方法 */
                    if (str_method_cmp(m, 'H', 'E', 'A', 'D')) {
                        rq->method = HTTP_HEAD;
                        break;
                    }
                    /* POST 方法 */
                    if (str_method_cmp(m, 'P', 'O', 'S', 'T')) {
                        rq->method = HTTP_POST;
                        break;
                    }

                    break;
                    
                default:
                    break;
                }

                state = spaces_before_uri;
                break;
            }

            if (ch < 'A' || ch > 'Z') {
                //DBG("");
                return HTTP_PARSE_INVALID_METHOD;
            }

            break;  /* case method */

        case spaces_before_uri:

            //DBG("spaces before uri.\n");
            switch (ch) {
            case ' ':
                break;

            case '/':
                rq->uri_start = p;
                state = after_slash_in_uri;
                break;

            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }

            break;  /* case spaces_before_uri */
        
        /* uri */
        case after_slash_in_uri:
            //DBG("after slash in uri.\n");
            switch (ch) {
            case ' ':
                rq->uri_end = p - 1;
                state = spaces_after_uri;
                break;
            
            case '?':
                rq->have_args = 1;
                rq->args_start = p + 1;
                break;

            default:
                break;
            }

            break;  /* case after_slash_in_uri */

        case spaces_after_uri:
            //DBG("spaces after uri.\n");
            switch (ch) {
            case ' ':
                break;
            
            case 'H':
                state = http_version_H;
                break;

            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }

            break;  /* case spaces_after_uri */

        /* http 版本号 */
        case http_version_H:
            //DBG("http version H.\n");
            switch (ch) {
            case 'T':
                state = http_version_HT;
                break;
            
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }

            break;  /* case http_version_H */
        
        case http_version_HT:
            //DBG("http version HT.\n");
            switch (ch) {
            case 'T':
                state = http_version_HTT;
                break;
            
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }

            break;  /* case http_version_HT */

        case http_version_HTT:
            //DBG("http version HTT.\n");
            switch (ch) {
            case 'P':
                state = http_version_HTTP;
                break;
            
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }

            break;  /* case http_version_HTT */

        case http_version_HTTP:
            //DBG("http version HTTP.\n");
            switch (ch) {
            case '/':
                state = http_version_first_major_digit;
                break;
            
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }

            break;  /* case http_version_HTTP */
        
        case http_version_first_major_digit:
            //DBG("http version first major digit.\n");
            if (ch < '0' || ch > '9') {
                return HTTP_PARSE_INVALID_REQUEST;
            }

            rq->http_version_major = ch - '0';
            state = http_version_major_digit;

            break;  /* case http_version_first_major_digit */
        
        case http_version_major_digit:
            //DBG("http version major digit.\n");
            if (ch == '.') {
                state = http_version_first_minor_digit;
                break;
            }

            if (ch < '0' || ch > '9') {
                return HTTP_PARSE_INVALID_REQUEST;
            }

            rq->http_version_major = rq->http_version_major * 10 + ch - '0';
            state = http_version_first_minor_digit;

            break;  /* case http_version_major_digit */

        case http_version_first_minor_digit:
            //DBG("http version first minor digit.\n");
            if (ch < '0' || ch > '9') {
                return HTTP_PARSE_INVALID_REQUEST;
            }

            rq->http_version_minor = ch - '0';
            state = http_version_minor_digit;

            break;  /* case http_version_first_minor_digit */

        case http_version_minor_digit:
            //DBG("http version minor digit.\n");
            
            if (ch < '0' || ch > '9') {
                switch (ch) {
                case ' ':
                    state = spaces_after_digit;
                    break;

                case CR:
                    state = almost_done;
                    break;

                case LF:
                    goto done;
                
                default:
                    return HTTP_PARSE_INVALID_REQUEST;
                }

                break;
            }

            rq->http_version_minor = rq->http_version_minor * 10 + ch - '0';
            state = spaces_after_digit;

            break;  /* case http_version_minor_digit */

        case spaces_after_digit:
            //DBG("apaces after digit.\n");
            switch (ch) {
            case ' ':
                break;
            
            case CR:
                state = almost_done;
                break;
            
            case LF:
                goto done;

            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }

            break;  /* case spaces_after_digit */

        case almost_done:
            //DBG("almost done.\n");
            switch (ch) {
            case LF:
                goto done;
            
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }

            break;  /* case almost_done */

        default:
            break;
        }
        
    }

    //DBG("for done.\n");

    rq->bufst = p;
    rq->state = state;

    return REQUEST_AGAIN;

done:

    rq->bufst = p + 1;
    rq->request_end = p;
    rq->state = start;

    return REQUEST_OK;
}

/*
 * 解析首部字段。
 */
int http_parse_request_headers(void* http_request) {
    http_request_t* rq;
    http_header_t* cur_header;
    unsigned char ch;
    unsigned char* p;

    rq = (http_request_t*)http_request;

    /* 维护一个状态机来解析 http 首部字段 */
    /* 格式：<name>: <value> CRLF */
    enum {
        start = 0,
        name,
        spaces_after_name,
        spaces_after_colon,
        value,
        spaces_after_value,
        cur_header_almost_done,
        almost_done
    } state;

    state = rq->state;

    //DBG("in parse headers: start for.\n");
    for (p = rq->bufst; p != rq->bufed; ++ p) {
        //DBG("state: %d\n", state);
        ch = *p;

        //DBG("ch = %c\n", ch);

        switch (state) {
        /* 开始 or 开始解析一行新的字段 */
        case start:
            //DBG("start.\n");
            switch (ch) {
            case CR:
                state = almost_done;    /* 如果连续两个 <CR><LF> ，表示首部字段结束 */
                break;

            case LF:
                goto done;

            default:
                rq->cur_header_name_start = p;
                state = name;
                break;
            }

            break;  /* case start */
        
        /* 首部字段名 */
        case name:
            //DBG("name.\n");
            switch (ch) {
            case ' ':
                rq->cur_header_name_end = p - 1;
                state = spaces_after_name;
                break;

            case ':':
                rq->cur_header_name_end = p - 1;
                state = spaces_after_colon;
                break;
            
            default:
                break;
            }

            break;  /* case name */

        case spaces_after_name:
            //DBG("spaces after name.\n");
            switch (ch) {
            case ' ':
                break;
            
            case ':':
                state = spaces_after_colon;
                break;

            default:
                return HTTP_PARSE_INVALID_HEADER;
            }

            break;  /* case spaces_after_name */

        case spaces_after_colon:
            //DBG("spaces after colon.\n");
            switch (ch) {
            case ' ':
                break;
            
            default:
                rq->cur_header_value_start = p;
                state = value;
                break;
            }

            break;  /* case spaces_after_colon */

        /* 首部字段值 */
        case value:
            //DBG("value.\n");
            switch (ch) {
            case ' ':
                rq->cur_header_value_end = p - 1;
                state = spaces_after_value;
                break;

            case CR:
                rq->cur_header_value_end = p - 1;
                state = cur_header_almost_done;
                break;
            
            case LF:
                rq->cur_header_value_end = p - 1;

                cur_header = (http_header_t*)malloc(sizeof(http_header_t));
                cur_header->header_name_start = rq->cur_header_name_start;
                cur_header->header_name_end = rq->cur_header_name_end;
                cur_header->header_value_start = rq->cur_header_value_start;
                cur_header->header_value_end = rq->cur_header_value_end;
                list_add(&(cur_header->list_node), &(rq->headers_list_head));

                state = start;  /* 继续解析下一行的首部字段 */
                break;
            
            default:
                break;
            }

            break;  /* case value */

        case spaces_after_value:
            switch (ch) {
            case ' ':
                break;
            
            case CR:
                state = cur_header_almost_done;
                break;

            case LF:
                cur_header = (http_header_t*)malloc(sizeof(http_header_t));
                cur_header->header_name_start = rq->cur_header_name_start;
                cur_header->header_name_end = rq->cur_header_name_end;
                cur_header->header_value_start = rq->cur_header_value_start;
                cur_header->header_value_end = rq->cur_header_value_end;
                list_add(&(cur_header->list_node), &(rq->headers_list_head));

                state = start;  /* 继续解析下一行的首部字段 */
                break;

            default:
                state = value;  /* 首部字段值可能会有空格 */
                break;
            }

            break;  /* case spaces_after_value */
            
        case cur_header_almost_done:
            switch (ch) {
            case LF:
                cur_header = (http_header_t*)malloc(sizeof(http_header_t));
                cur_header->header_name_start = rq->cur_header_name_start;
                cur_header->header_name_end = rq->cur_header_name_end;
                cur_header->header_value_start = rq->cur_header_value_start;
                cur_header->header_value_end = rq->cur_header_value_end;
                list_add(&(cur_header->list_node), &(rq->headers_list_head));

                state = start;  /* 继续解析下一行的首部字段 */
                break;
            
            case CR:
                break;

            default:
                return HTTP_PARSE_INVALID_HEADER;
            }

            break;  /* case cur_header_almost_done */

        case almost_done:
            switch (ch) {
            case LF:
                goto done;

            case CR:
                break;
            
            default:
                return HTTP_PARSE_INVALID_HEADER;
            }

            break;  /* case almost_done */

        default:
            break;
        }
    }

    //DBG("for done.\n");

    rq->bufst = p;
    rq->state = state;

    return REQUEST_AGAIN;

done:

    rq->bufst = p + 1;
    rq->state = start;

    return REQUEST_OK;
}