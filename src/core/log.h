/**
 * @author ttoobne
 * @date 2020/7/3
 */

#ifndef _LOG_H_
#define _LOG_H_

#define MAX_LOG 256

/* 六种日志级别 */
enum log_level {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} ;

#define log_trace(...)  log_log(LOG_TRACE, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define log_debug(...)  log_log(LOG_DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define log_info(...)   log_log(LOG_INFO,  __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define log_warn(...)   log_log(LOG_WARN,  __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define log_error(...)  log_log(LOG_ERROR, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define log_fatal(...)  log_log(LOG_FATAL, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#ifndef NLOG

/*
 * 日志的输出。
 */
void log_log(enum log_level level, const char* file, const char* function, const int line, const char* fmt, ...);

#else

#define log_log(level, fmt, ...) ((void)0)

#endif /* NLOG */

#endif /* _LOG_H_ */