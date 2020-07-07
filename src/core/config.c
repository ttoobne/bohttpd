/**
 * @author ttoobne
 * @date 2020/7/3
 */

#include "config.h"

#include "log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int check_name_value(config_t* config, char* name_st, char* name_ed, char* value_st, char* value_ed);
static long to_interger(char* st, char* ed);

/*
 * 解析配置文件，参数为文件名，返回 config_t 结构体指针。
 */
config_t* parse_configuration(char* filename) {
    config_t* config;
    char buf[CONFBUF_SIZE] = { '\0' };
    FILE* fp;
    char* p;
    char* name_st;
    char* name_ed;
    char* value_st;
    char* value_ed;
    char ch;
    int have_name;
    int have_value;
    int line;

    /* 维护状态机来解析配置文件 */
    /* 按行读取，每行格式 [<name> = <value>] [#[...]] */
    enum {
        start,
        name,
        spaces_before_equal_sign,
        equal_sign,
        spaces_before_value,
        value,
        end,
    } state;

    do {
        if ((config = (config_t*)malloc(sizeof(config_t))) == NULL) {
            log_error("config_t malloc failed.");
            return NULL;
        }

        /* 设置配置默认值 */
        config->threadpool = THREADPOOL_DEF;
        config->taskqueue = TASKQUEUE_DEF;
        memset(config->root, 0, sizeof(config->root));
        memset(config->defile, 0, sizeof(config->defile));
        strncpy(config->root, ROOT_DEF, 2);
        strncpy(config->defile, DEFILE_DEF, 10);
        config->timeout = TIMEOUT_DEF;
        config->port = PORT_DEF;

        /* 只读打开配置文件 */
        if ((fp = fopen(filename, "r")) == NULL) {
            log_error("config file open failed.");
            break;
        }

        state = start;
        line = 0;
        
        while (fgets(buf, sizeof(buf), fp) != NULL) {
            line ++ ;

            for (p = buf; *p != '\0'; ++ p) {
                ch = *p;

                switch (state) {
                case start:
                    have_name = 0;
                    have_value = 0;

                    if (ch == ' ' || ch == '\t') {
                        break;
                    }

                    if (ch == '#' || ch == '\n') {
                        state = end;
                        p -- ;
                        break;
                    }

                    if (!isalpha(ch) && !isdigit(ch)) {
                        if (!iscntrl(ch)) {
                            log_warn("line %d in configuration file: unrecognized syntax or character '%c'.", line, ch);
                        } else {
                            log_warn("line %d in configuration file: unrecognized syntax or character '0x%x'.", line, ch);
                        }
                        state = end;
                        p -- ;
                        break;
                    }

                    name_st = p;
                    state = name;

                    break;
                
                case name:
                    if (ch == ' ' || ch == '\t') {
                        name_ed = p - 1;
                        have_name = 1;
                        state = spaces_before_equal_sign;
                        break;
                    }

                    if (ch == '=') {
                        name_ed = p - 1;
                        have_name = 1;
                        state = spaces_before_value;
                        break;
                    }

                    if (!isalpha(ch) && !isdigit(ch)) {
                        if (!iscntrl(ch)) {
                            log_warn("line %d in configuration file: unrecognized syntax or character '%c'.", line, ch);
                        } else {
                            log_warn("line %d in configuration file: unrecognized syntax or character '0x%x'.", line, ch);
                        }
                        state = end;
                        p -- ;
                        break;
                    }

                    break;

                case spaces_before_equal_sign:
                    if (ch == ' ' || ch == '\t') {
                        break;
                    }

                    if (ch == '=') {
                        state = spaces_before_value;
                        break;
                    }
                    
                    if (!iscntrl(ch)) {
                        log_warn("line %d in configuration file: unrecognized syntax or character '%c'.", line, ch);
                    } else {
                        log_warn("line %d in configuration file: unrecognized syntax or character '0x%x'.", line, ch);
                    }
                    state = end;
                    p -- ;
                    break;

                case spaces_before_value:
                    if (ch == ' ' || ch == '\t') {
                        break;
                    }

                    value_st = p;
                    state = value;

                    break;

                case value:
                    if (ch == ' ' || ch == '\t' || ch == '#' || ch == '\n') {
                        value_ed = p - 1;
                        have_value = 1;
                        state = end;
                        p -- ;
                        break;
                    }

                    break;

                case end:
                    *(p + 1) = '\0';
                    state = start;

                    if (have_name != have_value) {
                        log_warn("line %d in configuration file: unrecognized syntax .", line);
                        break;
                    }

                    if (have_name) {
                        if (check_name_value(config, name_st, name_ed, value_st, value_ed) != 0) {
                            log_warn("line %d in configuration file: unrecognized identifier or value.", line);
                            break;
                        }
                    }

                    break;

                default:
                    break;
                }
            }
        }

        fclose(fp);
        fp = NULL;

        return config;

    } while(0);
    
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }

    config_destroy(config);

    return NULL;
    
}

int config_destroy(config_t* config) {
    if (config) {
        free(config);
    }
    
    return 0;
}

/*
 * 检查 name-value 是否合法，若合法则保存配置。
 */
static int check_name_value(config_t* config, char* name_st, char* name_ed, char* value_st, char* value_ed) {
    long ret;

    if (config == NULL || name_st == NULL || name_ed == NULL || value_st == NULL || value_ed == NULL) {
        return -1;
    }

    *(name_ed + 1) = '\0';
    *(value_ed + 1) = '\0';

    switch (name_ed - name_st + 1) {
    case 4:
        if (strncmp("root", name_st, name_ed - name_st + 1) == 0) {
            strncpy(config->root, value_st, sizeof(config->root));
            return 0;
        }
        
        if (strncmp("port", name_st, name_ed - name_st + 1) == 0) {
            if ((ret = (to_interger(value_st, value_ed))) < 0) {
                return -1;
            }

            if (ret > USHORT_MAX) {
                return -1;
            }

            config->port = ret;
            return 0;
        }

        break;
    
    case 6:
        if (strncmp("defile", name_st, name_ed - name_st + 1) == 0) {
            strncpy(config->defile, value_st, sizeof(config->defile));
            return 0;
        }

        break;

    case 7:
        if (strncmp("timeout", name_st, name_ed - name_st + 1) == 0) {
            if ((ret = (to_interger(value_st, value_ed))) < 0) {
                return -1;
            }

            config->timeout = ret;
            return 0;
        }

        break;

    case 9:
        if (strncmp("taskqueue", name_st, name_ed - name_st + 1) == 0) {
            if ((ret = (to_interger(value_st, value_ed))) < 0) {
                return -1;
            }

            if (ret > INT_MAX) {
                return -1;
            }

            config->taskqueue = ret;
            return 0;
        }

        break;

    case 10:
        if (strncmp("threadpool", name_st, name_ed - name_st + 1) == 0) {
            if ((ret = (to_interger(value_st, value_ed))) < 0) {
                return -1;
            }

            if (ret > INT_MAX) {
                return -1;
            }

            config->threadpool = ret;
            return 0;
        }

        break;
        
    default:
        break;
    }

    return -1;
}

/*
 * 将字符串转换为整数。
 */
static long to_interger(char* st, char* ed) {
    long result;
    char* p;

    result = 0;

    for (p = st; p <= ed; ++ p) {
        if (!isdigit(*p)) {
            return -1;
        }

        result = result * 10 + (*p - '0');

        if (result < 0) {
            return -1;
        }
    }

    return result;
}