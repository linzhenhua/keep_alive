#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/epoll.h>

#include "cJSON.h"
#include "ytk_daemon_log.h"

#define DIR "/home/dell/src/keep_alive/"
#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 1024

/*
 * 函数说明：重读配置文件
 * */
void ReadConfiguration()
{
    FILE *fp = NULL;
    int nLen = 0;
    char *pszFileBuf = NULL;

    cJSON *pszJson = NULL;
    cJSON *pszItem = NULL;

    char pszRemoteIp[20] = {0};
    char pszRemotePort[10] = {0};
    char pszLocalIp[20] = {0};
    char pszLocalPort[10] = {0};

    fp = fopen("keep_alive.conf", "rb");
    if(NULL == fp)
    {
        ytk_daemon_log(-1, "打开配置文件失败");
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "打开配置文件成功");
    }

    /*把文件指针移动到文件尾*/
    fseek(fp, 0, SEEK_END);    
    /*获取文件内容长度*/
    nLen = ftell(fp);
    /*把文件指针移动到文件首*/
    fseek(fp, 0, SEEK_SET);
    
    pszFileBuf = (char *)malloc((nLen + 1) * sizeof(char));
    if(NULL == pszFileBuf)
    {
        ytk_daemon_log(-1, "分配内存失败");
        fclose(fp);

        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "分配内存成功");
    }
    memset(pszFileBuf, 0, nLen + 1);

    fread(pszFileBuf, nLen, 1, fp);
    pszFileBuf[nLen] = '\0';
    fclose(fp);

    /*载入JSON字符串*/
    pszJson = cJSON_Parse(pszFileBuf);
    if(NULL == pszJson)
    {
        ytk_daemon_log(-1, "载入JSON字符串失败");
        free(pszFileBuf);
        pszFileBuf = NULL;
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "载入JSON字符串成功");
    }

    pszItem = cJSON_GetObjectItem(pszJson, "Remote_IP");
    if(NULL == pszItem)
    {
        ytk_daemon_log(-1, "解析Remote_IP失败");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "解析Remote_IP成功");
    }
    memcpy(pszRemoteIp, pszItem->valuestring, strlen(pszItem->valuestring));

    pszItem = cJSON_GetObjectItem(pszJson, "Remote_Port");
    if(NULL == pszItem)
    {
        ytk_daemon_log(-1, "解析Remote_Port失败");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "解析Remote_Port成功");
    }
    memcpy(pszRemotePort, pszItem->valuestring, strlen(pszItem->valuestring));
    
    pszItem = cJSON_GetObjectItem(pszJson, "Local_IP");    
    if(NULL == pszItem)
    {
        ytk_daemon_log(-1, "解析Local_IP失败");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "解析Local_IP成功");
    }
    memcpy(pszLocalIp, pszItem->valuestring, strlen(pszItem->valuestring));

    pszItem = cJSON_GetObjectItem(pszJson, "Local_Port");
    if(NULL == pszItem)
    {
        ytk_daemon_log(-1, "解析Local_Port失败");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "解析Local_Port成功");
    }
    memcpy(pszLocalPort, pszItem->valuestring, strlen(pszItem->valuestring));

    /*test
    printf("pszRemoteIp: %s\n", pszRemoteIp);
    printf("pszRemotePort: %s\n", pszRemotePort);
    printf("pszLocalIp: %s\n", pszLocalIp);
    printf("pszLocalPort: %s\n", pszLocalPort);
    test*/
    
    cJSON_Delete(pszJson);
    pszJson = NULL;
    free(pszFileBuf);
    pszFileBuf = NULL;

    ytk_daemon_log(0, "重新读取配置文件成功");
}

/*
 * 函数说明：SIGHUP处理函数
 * */
void Hup(int s)
{
    struct sigaction sa;

    /*Learn the new rules*/
    ReadConfiguration();
    
    /*And reinstall the signal handler*/
    sa.sa_handler = Hup;    
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGHUP, &sa, NULL) < 0)
    {
        ytk_daemon_log(-1, "监听SIGHUP信号失败");
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "监听SIGHUP信号成功");
    }
}

/*
 * 函数说明：SIGPIPE处理函数
 * */
void Plumber(int s)
{
    struct sigaction sa;

    /*just reinstall*/
    sa.sa_handler = Plumber;    
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGPIPE, &sa, NULL) < 0)
    {
        ytk_daemon_log(-1, "监听SIGPIPE信号失败");
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "监听SIGPIPE信号成功");
    }
}

/*
 * 函数说明：SIGTERM处理函数，关闭日志文件
 * */
void Term(int s)
{
    ytk_daemon_log(0, "进程结束\n");
    exit(0);
}

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
void InitDaemon()
{
    int i, fd;
    pid_t pid;
    struct sigaction sa;
    int res;

    /*转为后台进程*/
    if( (pid = fork()) < 0 )
    {
        ytk_daemon_log(-1, "创建守护进程失败");
        exit(1);
    }
    else if(0 != pid)
    {
        ytk_daemon_log(0, "父进程退出");
        exit(0);
    }

    /*开启新的会话组，成为会话组长和进程组长*/
    setsid();
    
    /*使其不再是会话组长，不能开启终端*/
    if( (pid = fork()) < 0 )
    {
        ytk_daemon_log(-1, "创建守护进程失败");
        exit(1);
    }
    else if(0 != pid) 
    {
        ytk_daemon_log(0, "父进程退出");
        exit(0);
    }

    /*关闭已经打开的文件描述符，避免浪费系统资源*/
    for(i = 0; i < NOFILE; ++i)
    {
        close(i);
    }

    /*更改工作目录*/
    res = chdir(DIR);
    if( res < 0 )
    {
        ytk_daemon_log(-1, "更改工作目录失败");
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "更改工作目录成功");
    }

    /*重设文件掩码，使文件操作权限不再受父进程影响*/
    umask(0);

    /*重定向输入输出*/
    fd = open("/dev/null", O_RDWR);
    if(fd < 0)
    {
        ytk_daemon_log(-1, "重定向输入输出失败");
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "重定向输入输出成功");
    }

    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);

    /*设置SIGHUP的处理函数为Hup，重读配置文件*/
    sa.sa_handler = Hup;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGHUP, &sa, NULL) < 0 )
    {
        ytk_daemon_log(-1, "监听SIGHUP信号失败");
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "监听SIGHUP信号成功");
    }

    /*忽略SIGCHLD信号，避免大量僵尸进程占用系统资源*/
    /*设置SIGCHLD的处理函数为SIG_ING*/
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGCHLD, &sa, NULL) < 0 )
    {
        ytk_daemon_log(-1, "监听SIGCHLD信号失败");
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "监听SIGCHLD信号成功");
    }

    /*设置SIGPIPE的处理函数为Plumber*/
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = Plumber;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGPIPE, &sa, NULL) < 0 )
    {
        ytk_daemon_log(-1, "监听SIGPIPE信号失败");
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "监听SIGPIPE信号成功");
    }

    /*设置SIGTERM的处理函数为Term*/
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = Term;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGTERM, &sa, NULL) < 0 )
    {
        ytk_daemon_log(-1, "监听SIGTERM信号失败");
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "监听SIGTERM信号成功");
    }
}

/*
 * 函数说明：重新读取配置文件
 * */
void ReadConf(char *pszLocalIp, char *pszLocalPort, char *pszRemoteIp, char *pszRemotePort)
{
    FILE *fp = NULL;
    int nLen = 0;
    char *pszFileBuf = NULL;

    cJSON *pszJson = NULL;
    cJSON *pszItem = NULL;

    fp = fopen("keep_alive.conf", "rb");
    if(NULL == fp)
    {
        ytk_daemon_log(-1, "打开配置文件失败");
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "打开配置文件成功");
    }

    /*把文件指针移动到文件尾*/
    fseek(fp, 0, SEEK_END);    
    /*获取文件内容长度*/
    nLen = ftell(fp);
    /*把文件指针移动到文件首*/
    fseek(fp, 0, SEEK_SET);
    
    pszFileBuf = (char *)malloc((nLen + 1) * sizeof(char));
    if(NULL == pszFileBuf)
    {
        ytk_daemon_log(-1, "分配内存失败");
        fclose(fp);

        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "分配内存成功");
    }
    memset(pszFileBuf, 0, nLen + 1);

    fread(pszFileBuf, nLen, 1, fp);
    pszFileBuf[nLen] = '\0';
    fclose(fp);

    /*载入JSON字符串*/
    pszJson = cJSON_Parse(pszFileBuf);
    if(NULL == pszJson)
    {
        ytk_daemon_log(-1, "载入JSON字符串失败");
        free(pszFileBuf);
        pszFileBuf = NULL;
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "载入JSON字符串成功");
    }

    pszItem = cJSON_GetObjectItem(pszJson, "Remote_IP");
    if(NULL == pszItem)
    {
        ytk_daemon_log(-1, "解析Remote_IP失败");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "解析Remote_IP成功");
    }
    memcpy(pszRemoteIp, pszItem->valuestring, strlen(pszItem->valuestring));

    pszItem = cJSON_GetObjectItem(pszJson, "Remote_Port");
    if(NULL == pszItem)
    {
        ytk_daemon_log(-1, "解析Remote_Port失败");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "解析Remote_Port成功");
    }
    memcpy(pszRemotePort, pszItem->valuestring, strlen(pszItem->valuestring));
    
    pszItem = cJSON_GetObjectItem(pszJson, "Local_IP");    
    if(NULL == pszItem)
    {
        ytk_daemon_log(-1, "解析Local_IP失败");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "解析Local_IP成功");
    }
    memcpy(pszLocalIp, pszItem->valuestring, strlen(pszItem->valuestring));

    pszItem = cJSON_GetObjectItem(pszJson, "Local_Port");
    if(NULL == pszItem)
    {
        ytk_daemon_log(-1, "解析Local_Port失败");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
        exit(1);
    }
    else
    {
        ytk_daemon_log(0, "解析Local_Port成功");
    }
    memcpy(pszLocalPort, pszItem->valuestring, strlen(pszItem->valuestring));
    
    cJSON_Delete(pszJson);
    pszJson = NULL;
    free(pszFileBuf);
    pszFileBuf = NULL;
}

/*
 * 函数说明：将文件描述符设置成非阻塞
 * */
int SetNonBlocking(int fd)
{
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);

    return oldOption;
}

/*
 * 函数说明：将文件描述符fd上的EPOLLIN注册到epollfd指示的epoll内核事件中，
 * 参数enableEt指定是否对fd启用ET模式
 * */
void AddFd(int epollfd, int fd, int enableEt)
{
    epoll_event event;
    event.data.fd = fd;
    event.events =  EPOLLIN;

    if(1 == enableEt)
    {
        event.events |= EPOLLET;
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    SetNonBlocking(fd);
}

/*
 * 函数说明：删除epoll上注册的事件
 * */
void RemoveFd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

/*
 * 函数说明：保持长连接
 * */
void KeepAlive()
{
    int res;

    /*
     * 配置文件里的地址
     * 中行IP：Local_IP
     * 中行端口：Local_Port
     * 联合电子IP：Remote_IP
     * 联合电子Port：Remote_Port
     * */
    char pszPbcIp[20] = {0};
    char pszPbcPort[10] = {0};
    char pszEServeIp[20] = {0};
    char pszEServePort[10] = {0};

    /*
     * 读取配置文件里的IP和端口
     * 中行的数据发送到pszPbcIp,pszPbcPort
     * 联合电子的IP和端口pszEServeIp,pszEServePort
     * */
    ReadConf(pszPbcIp, pszPbcPort, pszEServeIp, pszEServePort);

    /*创建pbcAddress,作为服务端监听中行是否有数据过来*/
    struct sockaddr_in pbcAddress;
    bzero(&pbcAddress, sizeof(pbcAddress));
    pbcAddress.sin_family = AF_INET;
    inet_pton(AF_INET, pszPbcIp, &pbcAddress.sin_addr);   
    pbcAddress.sin_port = htons( atoi(pszPbcPort) );

    /*创建eServeAddress,作为客户端用于连接联合电子的服务器*/
    struct sockaddr_in eServeAddress;
    bzero(&eServeAddress, sizeof(eServeAddress));
    eServeAddress.sin_family = AF_INET;
    inet_pton(AF_INET, pszEServeIp, &eServeAddress.sin_addr);
    eServeAddress.sin_port = htons( atoi(pszEServePort) );

    /*创建一个服务端监听pszPbcIp,pszPbcPort*/
    /*建立socket*/
    int pbcfd = socket(PF_INET, SOCK_STREAM, 0);
    if(pbcfd >= 0)
    {
        ytk_daemon_log(0, "建立服务端socket成功");
    }
    else
    {
        ytk_daemon_log(-1, "建立服务端socket失败");

        exit(1);
    }

    /*绑定socket*/
    res = bind(pbcfd, (struct sockaddr *)&pbcAddress, sizeof(pbcAddress));
    if(res != -1)
    {
        ytk_daemon_log(0, "绑定服务端socket成功");
    }
    else
    {
        ytk_daemon_log(-1, "绑定服务端socket失败");
        close(pbcfd);

        exit(1);
    }

    /*监听socket*/
    res = listen(pbcfd, 5);
    if(res != -1)
    {
        ytk_daemon_log(0, "监听服务端socket成功");
    }
    else
    {
        ytk_daemon_log(-1, "监听服务端socket失败");
        close(pbcfd);

        exit(1);
    }

    /*创建一个客户端socket连接pszEServeIp,pszEServePort*/
    /*建立socket*/
    int eServefd = socket(PF_INET, SOCK_STREAM, 0);
    if(eServefd >= 0)
    {
        ytk_daemon_log(0, "建立客户端socket成功");
    }
    else
    {
        ytk_daemon_log(-1, "建立客户端socket失败");
        close(pbcfd);

        exit(1);
    }
    res = connect(eServefd, (struct sockaddr *)&eServeAddress, sizeof(eServeAddress));
    if(res != -1)
    {
        ytk_daemon_log(0, "连接联合电子服务器成功");
    }
    else
    {
        ytk_daemon_log(-1, "连接联合电子服务器失败");
        close(pbcfd);
        close(eServefd);

        exit(1);
    }

    /*中行发送过来的数据保存到这里*/
    char pszPbcRecvBuf[BUFFER_SIZE];
    /*联合电子发送过来的数据保存到这里*/
    char pszEServeRecvBuf[BUFFER_SIZE];

    epoll_event events[MAX_EVENT_NUMBER];

    int epollfd = epoll_create(5);
    if(-1 != epollfd)
    {
        ytk_daemon_log(0, "创建epoll成功");
    }
    else
    {
        ytk_daemon_log(-1, "创建epoll失败");
        close(pbcfd);
        close(eServefd);

        exit(1);
    }

    AddFd(epollfd, pbcfd, 1);
    AddFd(epollfd, eServefd, 1);

    int connfd;
    int epollRes;

    while(1)
    {
        epollRes = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if(epollRes < 0)
        {
            ytk_daemon_log(-1, "epoll failure");
            break;
        }
        for(int i = 0; i < epollRes; ++i)
        {   
            int sockfd = events[i].data.fd;

            /*中行有连接过来*/
            if(sockfd == pbcfd)
            {
                /*定义客户端地址*/
                struct sockaddr_in clientAddress;
                socklen_t nClientAddressLen = sizeof(clientAddress);

                /*接受客户端连接*/
                connfd = accept(pbcfd, (struct sockaddr *)&clientAddress, &nClientAddressLen);
                if(connfd < 0)
                {
                    ytk_daemon_log(-1, "服务端接受客户连接失败");
                    continue;
                }
                else
                {
                    ytk_daemon_log(0, "服务端接受客户连接成功");
                }
                AddFd(epollfd, connfd, 1);

                /*打印连接到的IP和port*/
                char clientAddressIpTmp[INET_ADDRSTRLEN + 1];
                memset(clientAddressIpTmp, 0, sizeof(clientAddressIpTmp));
                inet_ntop(AF_INET, &clientAddress.sin_addr, clientAddressIpTmp, INET_ADDRSTRLEN);
                ytk_daemon_log(0, "连接到的客户端IP为：");
                ytk_daemon_log(0, clientAddressIpTmp);
                unsigned int clientAddressPortTmp = ntohs(clientAddress.sin_port);
                char portTmp[10];
                memset(portTmp, 0, sizeof(portTmp));
                sprintf(portTmp, "%d", clientAddressPortTmp);
                ytk_daemon_log(0, "连接到的客户端port为：");
                ytk_daemon_log(0, portTmp);
            }
            /*联合电子有数据过来*/
            else if( sockfd == eServefd )
            {
                /*ET模式下需要循环读取数据*/
                while(1)
                {
                    memset(pszEServeRecvBuf, '\0', BUFFER_SIZE); 
                    res = recv(eServefd, pszEServeRecvBuf, BUFFER_SIZE-1, 0);
                    if(res < 0)
                    {
                        /*对于非阻塞IO，下面的条件成立表示数据已经全部读取完毕，此后，epoll就能再次触发sockfd上的EPOLLIN事件，以驱动下一次读操作*/
                        if( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
                        {
                            ytk_daemon_log(-1, "接收联合电子的数据完毕");
                            break;
                        }
                    }
                    else if(0 == res)
                    {
                        ytk_daemon_log(-1, "联合电子关闭了连接，守护进程将在10秒后重新连接");

                        sleep(10);
                        res = connect(eServefd, (struct sockaddr *)&eServeAddress, sizeof(eServeAddress));
                        if(res != -1)
                        {
                            ytk_daemon_log(0, "重新连接联合电子服务器成功");
                        }
                        else
                        {
                            ytk_daemon_log(-1, "重新连接联合电子服务器失败");
                        }
                        break;
                    }
                    else
                    {
                        /*把接收到的数据转发到中行的connfd*/
                        /*先判断是否是心跳包0000,是就回一个心跳，不是就转发数据到connfd*/
                        ytk_daemon_log(0, pszEServeRecvBuf);
                        res = strncmp("0000", pszEServeRecvBuf, 4);
                        if(0 == res)
                        {
                            /*是心跳包，返回一个心跳包0000*/
                            res = send(eServefd, "0000", strlen("0000"), 0);
                            if( res <= 0 )
                            {
                                ytk_daemon_log(-1, "发送心跳包失败");
                            }
                            ytk_daemon_log(0, "返回一个心跳包");
                        }
                        else
                        {
                            /*把数据发送到中行的客户端connfd,然后清空缓存，继续接收联合电子的数据*/
                            res = send(connfd, pszEServeRecvBuf, sizeof(pszEServeRecvBuf)-1, 0);
                            if( res <= 0 )
                            {
                                ytk_daemon_log(-1, "发送到中行数据出错");
                            }
                            else
                            {
                                ytk_daemon_log(0, "发送到中行数据成功");
                            }
                        }
                    }
                }/* end of while(1)*/
            }
            /*中行有数据过来*/
            else if(sockfd == connfd)
            {
                /*ET模式下需要循环读取数据*/
                while(1)
                {
                    memset(pszPbcRecvBuf, '\0', BUFFER_SIZE); 
                    res = recv(connfd, pszPbcRecvBuf, BUFFER_SIZE-1, 0);
                    if(res < 0)
                    {
                        /*对于非阻塞IO，下面的条件成立表示数据已经全部读取完毕，此后，epoll就能再次触发sockfd上的EPOLLIN事件，以驱动下一次读操作*/
                        if( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
                        {
                            ytk_daemon_log(-1, "接收中行的数据完毕");
                            break;
                        }
                        else
                        {
                            ytk_daemon_log(-1, "接收数据出错");
                            /*关闭connfd*/
                            RemoveFd(epollfd, connfd);
                            break;
                        }
                    }
                    else if(0 == res)
                    {
                        ytk_daemon_log(-1, "中行关闭了连接");
                        /*关闭connfd*/
                        RemoveFd(epollfd, connfd);
                        break;
                    }
                    else
                    {
                        /*把接收到的数据转发到联合电子的eServefd*/
                        ytk_daemon_log(0, pszPbcRecvBuf);
                        /*把数据发送到联合电子的eServefd,然后清空缓存，继续接收联合电子的数据*/
                        res = send(eServefd, pszPbcRecvBuf, sizeof(pszPbcRecvBuf)-1, 0);
                        if( res <= 0 )
                        {
                            ytk_daemon_log(-1, "发送到联合电子数据出错");
                        }
                        else
                        {
                            ytk_daemon_log(0, "发送到联合电子数据成功");
                        }
                    }
                }
            }
        }/*end of for(int i = 0; i < epollRes; ++i) */
    }/* end of while(1)*/

    RemoveFd(epollfd, pbcfd);
    RemoveFd(epollfd, eServefd);
}
