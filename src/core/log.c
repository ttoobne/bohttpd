/**
 * @author ttoobne
 * @date 2020/7/3
 */

#include "log.h"

#ifndef NLOG

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static const char* level_strings[] = {
    [LOG_TRACE ] = "TRACE",
    [LOG_DEBUG ] = "DEBUG",
    [LOG_INFO  ] = "INFO",
    [LOG_WARN  ] = "WARN",
    [LOG_ERROR ] = "ERROR",
    [LOG_FATAL ] = "FATAL"
};

static const char* level_colors[] = {
    [LOG_TRACE ] = "\x1b[94;1m",
    [LOG_DEBUG ] = "\x1b[36;1m",
    [LOG_INFO  ] = "\x1b[37;1m",
    [LOG_WARN  ] = "\x1b[33;1m",
    [LOG_ERROR ] = "\x1b[31;1m",
    [LOG_FATAL ] = "\x1b[31;1m"
};

/*
 * 日志的输出。
 */
void log_log(enum log_level level, const char* file, const char* function, const int line, const char* fmt, ...) {
    char log[MAX_LOG] = { '\0' };
    char date[18] = { '\0' };
    struct tm* tm;
    time_t now;
    va_list args;

    now = time(NULL);
    tm = localtime(&now);
    strftime(date, sizeof(date), "%y-%m-%d %H:%M:%S", tm);

    va_start(args, fmt);
    vsnprintf(log, sizeof(log), fmt, args);
    va_end(args);

    fprintf(stderr, "[%17s][%s(%d):%s]%s%s: \x1B[0m%s\n", 
                    date, file, line, function, level_colors[level], level_strings[level], log);
}

#endif /* NLOG */