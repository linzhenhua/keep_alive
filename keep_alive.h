#ifndef KEEP_ALIVE_H
#define KEEP_ALIVE_H

/*
* exit（0）：正常运行程序并退出程序；
* exit（1）：非正常运行导致退出程序；
*/

/*
 * 函数说明：重读配置文件
 * */
void ReadConfiguration();

/*
 * 函数说明：SIGHUP处理函数
 * */
void Hup(int s);

/*
 * 函数说明：SIGPIPE处理函数
 * */
void Plumber(int s);

/*
 * 函数说明：SIGTERM处理函数，关闭日志文件
 * */
void Term(int s);

/*
 * 函数说明：启动一个守护进程
 * 编程规则：
 * 1、首先要做的是调用umask将文件模式创建屏蔽字设置为0
 * 2、调用fork，然后使父进程退出（exit）
 * 3、调用setsid以创建一个新会话
 * 4、将当前工作目录更改为根目录
 * 5、关闭不再需要的文件描述符
 * 6、某些守护进程打开/dev/null使其具有文件描述符0,1,2，这样任何一个试图读标准输入、
 * 写标准输出或标准出错的库例程都不会产生任何效果
 * */
void InitDaemon();

/*
 * 函数说明：重新读取配置文件
 * */
void ReadConf(char *pszLocalIp, char *pszLocalPort, char *pszRemoteIp, char *pszRemotePort);

/*
 * 函数说明：将文件描述符设置成非阻塞
 * */
int SetNonBlocking(int fd);

/*
 * 函数说明：将文件描述符fd上的EPOLLIN注册到epollfd指示的epoll内核事件中，
 * 参数enableEt指定是否对fd启用ET模式
 * */
void AddFd(int epollfd, int fd, int enableEt);

/*
 * 函数说明：删除epoll上注册的事件
 * */
void RemoveFd(int epollfd, int fd);

/*
 * 函数说明：保持长连接
 * */
void KeepAlive();

#endif
