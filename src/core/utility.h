/**
 * @author ttoobne
 * @date 2020/6/5
 */

#ifndef _UTILITY_H_
#define _UTILITY_H_

/*
 * 设置文件描述符为非阻塞，返回旧的选项。
 */
int set_nonblocking(int fd);

/*
 * 忽略 SIGPIPE 信号。
 */
int ignore_sigpipe();

/*
 * 初始化进程为守护进程。
 */
void daemonize(const char *cmd);

#endif /* _UTILITY_H_ */
