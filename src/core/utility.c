/**
 * @author ttoobne
 * @date 2020/6/5
 */

#include "utility.h"

#include "log.h"

#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>

/*
 * 设置文件描述符为非阻塞，返回旧的选项。
 */
int set_nonblocking(int fd)
{
    int old_option;
    int new_option;

    if ((old_option = fcntl(fd, F_GETFL)) == -1)
    {
        log_error("get fd option failed.");
        return -1;
    }

    new_option = old_option | O_NONBLOCK;

    if (fcntl(fd, F_SETFL, new_option) == -1)
    {
        log_error("set fd option failed.");
        return -1;
    }

    return old_option;
}

/*
 * 忽略 SIGPIPE 信号。
 */
int ignore_sigpipe()
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;

    if (sigaction(SIGPIPE, &sa, NULL))
    {
        log_error("ignore SIGPIPE signal failed.");
        return -1;
    }

    return 0;
}

/*
 * 初始化进程为守护进程。
 */
void daemonize(const char *cmd)
{
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;
    int i;
    int fd0;
    int fd1;
    int fd2;

    /* 设置屏蔽字 */
    umask(0);

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        log_error("%s: can't get file limit", cmd);
    }

    /* 调用 fork() ，使父进程 exit */
    if ((pid = fork()) < 0) {
        log_error("%s: can't fork", cmd);
    } else if (pid != 0) {
        exit(0);
    }

    /* 创建一个新会话 */
    setsid();

    /* 确保没有控制终端 */
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        log_error("%s: can't ignore SIGHUP", cmd);
    }
        
    if ((pid = fork()) < 0) {
        log_error("%s: can't fork", cmd);
    } else if (pid != 0) {
        exit(0);
    }
        

    /* 切换当前工作目录为根目录 */
    if (chdir("/") < 0) {
        log_error("%s: can't change directory to /", cmd);
    }

    /* 关闭所有文件描述符 */
    if (rl.rlim_max == RLIM_INFINITY) {
        rl.rlim_max = 1024;
    }
        
    for (i = 0; i < rl.rlim_max; i++) {
        close(i);
    }

    /* 在终端看不到守护进程的输出，用户输入不会被守护进程读取 */
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_ERR, "unexpected file descriptors %d %d %d",
               fd0, fd1, fd2);
        exit(1);
    }
}
