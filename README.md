# bohttpd
一个作为学习与实践用途的、简单的并发 http 服务端。

# 背景
疫情期间最好不要出门，于是宅在家里读了 W.Richard Stevens 先生的几本著作（TCP/IPv1、APUE、UNPv1），一遍下来之后觉得很多内容比较多且细致，读起来难以消化，遂决定自己尝试着实现一个http服务端，在实践过程中遇到问题在去书中寻找答案可能效果会更好。
整个实现过程将近两个月时间，过程中阅读了不少开源代码，也学到了很多。
本 http 服务端采用了非阻塞I/O，对于已连接描述符若当前接收缓冲中没有数据则读到 EAGAIN 直接返回去执行其他可读事件，同时多路复用事件循环采用 epoll 实现，配合使用线程池并发地执行请求。
代码注释十分详尽，但由于英文水平一般，绝大部分注释都是使用的中文。

开发&测试环境为 CentOS release 6.10 (Final)。

# 编译&运行
1. make
```
make
./bohttpd
```
2. cmake
```
cd build
cmake .
make
./bohttpd
```

# 支持
1. 目前仅支持 GET 方法，静态的 http 请求，能够分析简单的请求首部。
2. 支持 http 持久连接，以在频繁的读写时维持连接不关闭。
3. 支持定时器提供定时机制，定时清理超时的持久连接。定时器参考 nginx 采用红黑树实现。
4. 支持自定义配置文件，可以指定线程池大小、持久连接的超时时间、默认主目录等。
5. 实现了简易的日志库。

# 更多选项
```
Usage: bohttpd [option]... 
  -?,-h,--help                 this help.
  -c,--config <filename>       set configuration file. (default: "./bohttpd.conf")
  -V,--version                 show version and exit.
  -t,--testconf                test configuration and exit.
```

# License
MIT (c) 2020, ttoobne.
