#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "bcd2hex.h"
#include "cJSON.h"

#define DaemonLog(error_code, error) \
    _DaemonLog(error_code, error, __FILE__, __LINE__, __FUNCTION__)

#define DIR "/home/dell/src/keep_alive"
#define MAX_EVENT_NUMBER 5
#define BUFFER_SIZE 1024

int g_server_fd = -1;
int g_client_fd = -1;
int g_epoll_fd = -1;
FILE *g_file_fd = NULL;

/*
 * 函数说明：记录守护进程执行过程
 * 参数1：错误码
 * 参数2：错误信息
 * 参数3：文件名
 * 参数4：行号
 * 参数5：出错函数
 * 返回值：无
 * */
void _DaemonLog(const int error_code, const char *error_msg, const char *file_name, int line, const char *func)
{
    struct timeval tv;
    struct tm *now_time;

    memset(&tv, 0, sizeof(struct timeval));

    gettimeofday(&tv, NULL);

    now_time = localtime(&tv.tv_sec);

    char log_name[30] = {'\0'};

    //日志名称
    sprintf(log_name, "daemon_%04d%02d%02d.log", now_time->tm_year+1900, now_time->tm_mon+1, now_time->tm_mday);

    //打开文件
    if(NULL == g_file_fd)
    {
        g_file_fd = fopen(log_name, "a");
        if(NULL == g_file_fd)
        {
            exit(1);
        }
    }

    fprintf(g_file_fd, "%d-%02d-%02d-%02d-%02d-%02d-%.04d_%s_%d_%s_%d : %s\n",
            now_time->tm_year + 1900,
            now_time->tm_mon + 1,
            now_time->tm_mday,
            now_time->tm_hour,
            now_time->tm_min,
            now_time->tm_sec,
            (int)tv.tv_usec,
            file_name,
            line,
            func,
            error_code,
            error_msg);

    //fflush(g_file_fd);

    fclose(g_file_fd);
    g_file_fd = NULL;
}

/*
 * 函数说明：重读配置文件
 * 参数：无
 * 返回值：无
 * */
void ReReadConfiguration()
{
    FILE *fp = NULL;
    int file_len = 0;
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
        DaemonLog(-1, "打开配置文件失败");
    }
    else
    {
        DaemonLog(0, "打开配置文件成功");
    }

    /*把文件指针移动到文件尾*/
    fseek(fp, 0, SEEK_END);
    /*获取文件内容长度*/
    file_len = ftell(fp);
    /*把文件指针移动到文件首*/
    fseek(fp, 0, SEEK_SET);

    pszFileBuf = (char *)malloc((file_len + 1) * sizeof(char));
    if(NULL == pszFileBuf)
    {
        DaemonLog(-1, "分配内存失败");
        fclose(fp);

        exit(1);
    }
    else
    {
        DaemonLog(0, "分配内存成功");
    }
    memset(pszFileBuf, 0, file_len + 1);

    fread(pszFileBuf, file_len, 1, fp);
    pszFileBuf[file_len] = '\0';
    fclose(fp);

    /*载入JSON字符串*/
    pszJson = cJSON_Parse(pszFileBuf);
    if(NULL == pszJson)
    {
        DaemonLog(-1, "载入JSON字符串失败");
        free(pszFileBuf);
        pszFileBuf = NULL;
    }
    else
    {
        DaemonLog(0, "载入JSON字符串成功");
    }

    pszItem = cJSON_GetObjectItem(pszJson, "Remote_IP");
    if(NULL == pszItem)
    {
        DaemonLog(-1, "解析Remote_IP失败");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
    }
    else
    {
        DaemonLog(0, "解析Remote_IP成功");
    }
    memcpy(pszRemoteIp, pszItem->valuestring, strlen(pszItem->valuestring));

    pszItem = cJSON_GetObjectItem(pszJson, "Remote_Port");
    if(NULL == pszItem)
    {
        DaemonLog(-1, "解析Remote_Port失败");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
    }
    else
    {
        DaemonLog(0, "解析Remote_Port成功");
    }
    memcpy(pszRemotePort, pszItem->valuestring, strlen(pszItem->valuestring));

    pszItem = cJSON_GetObjectItem(pszJson, "Local_IP");
    if(NULL == pszItem)
    {
        DaemonLog(-1, "解析Local_IP失败");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
    }
    else
    {
        DaemonLog(0, "解析Local_IP成功");
    }
    memcpy(pszLocalIp, pszItem->valuestring, strlen(pszItem->valuestring));

    pszItem = cJSON_GetObjectItem(pszJson, "Local_Port");
    if(NULL == pszItem)
    {
        DaemonLog(-1, "解析Local_Port失败");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
    }
    else
    {
        DaemonLog(0, "解析Local_Port成功");
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

    DaemonLog(0, "重新读取配置文件成功");
}

/*
 * 函数说明：SIGHUP处理函数（重读配置文件）
 * 参数：未使用
 * 返回值：无
 * */
void Hup(int s)
{
    DaemonLog(0, "正在重新读取配置文件");

    struct sigaction sa;

    /*Learn the new rules*/
    ReReadConfiguration();

    /*And reinstall the signal handler*/
    sa.sa_handler = Hup;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGHUP, &sa, NULL) < 0)
    {
        DaemonLog(-1, "重新监听SIGHUP信号失败");
    }
    else
    {
        DaemonLog(0, "重新监听SIGHUP信号成功");
    }
}

/*
 * 函数说明：SIGPIPE处理函数（往关闭的socket文件描述符写或者读数据触发SIGPIPE信号）
 * 参数：未使用
 * 返回值：无
 * */
void Plumber(int s)
{
    DaemonLog(0, "往关闭的socket文件描述符读或者写数据");

    struct sigaction sa;

    /*just reinstall*/
    sa.sa_handler = Plumber;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGPIPE, &sa, NULL) < 0)
    {
        DaemonLog(-1, "重新监听SIGPIPE信号失败");
    }
    else
    {
        DaemonLog(0, "重新监听SIGPIPE信号成功");
    }
}

/*
 * 函数说明：SIGTERM处理函数，（kill守护进程时触发）释放所有资源
 * 参数：未使用
 * 返回值：无
 * */
void Term(int s)
{
    if(g_server_fd != -1)
    {
        close(g_server_fd);
        g_server_fd = -1;
    }
    if(g_client_fd != -1)
    {
        close(g_client_fd);
        g_client_fd = -1;
    }
    if(g_file_fd != NULL)
    {
        fclose(g_file_fd);
        g_file_fd = NULL;
    }

    DaemonLog(0, "进程结束\n");
    exit(0);
}

/*
 * 编程规则：
 * 1、首先要做的是调用umask将文件模式创建屏蔽字设置为0
 * 2、调用fork，然后使父进程退出（exit）
 * 3、调用setsid以创建一个新会话
 * 4、将当前工作目录更改为根目录
 * 5、关闭不再需要的文件描述符
 * 6、某些守护进程打开/dev/null使其具有文件描述符0,1,2，这样任何一个试图读标准输入、
 * 写标准输出或标准出错的库例程都不会产生任何效果
 *
 * 函数说明：启动一个守护进程
 * 参数：未使用
 * 返回值：无
 * */
void InitDaemon()
{
    int i, fd;
    pid_t pid;
    struct sigaction sa;

    /*转为后台进程*/
    if( (pid = fork()) < 0 )
    {
        DaemonLog(-1, "创建守护进程失败");
        exit(1);
    }
    else if(0 != pid)
    {
        DaemonLog(0, "父进程退出");
        exit(0);
    }

    /*开启新的会话组，成为会话组长和进程组长*/
    setsid();

    /*使其不再是会话组长，不能开启终端*/
    if( (pid = fork()) < 0 )
    {
        DaemonLog(-1, "创建守护进程失败");
        exit(1);
    }
    else if(0 != pid)
    {
        DaemonLog(0, "父进程退出");
        exit(0);
    }

    /*关闭已经打开的文件描述符，避免浪费系统资源*/
    for(i = 0; i < NOFILE; ++i)
    {
        close(i);
    }

    /*更改工作目录*/
    int res;
    res = chdir(DIR);
    if( res < 0 )
    {
        DaemonLog(-1, "更改工作目录失败");
        exit(1);
    }
    else
    {
        DaemonLog(0, "更改工作目录成功");
    }

    /*重设文件掩码，使文件操作权限不再受父进程影响*/
    umask(0);

    /*重定向输入输出*/
    fd = open("/dev/null", O_RDWR);
    if(fd < 0)
    {
        DaemonLog(-1, "重定向输入输出失败");
        exit(1);
    }
    else
    {
        DaemonLog(0, "重定向输入输出成功");
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
        DaemonLog(-1, "监听SIGHUP信号失败");
        exit(1);
    }
    else
    {
        DaemonLog(0, "监听SIGHUP信号成功");
    }

    /*忽略SIGCHLD信号，避免大量僵尸进程占用系统资源*/
    /*设置SIGCHLD的处理函数为SIG_ING*/
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGCHLD, &sa, NULL) < 0 )
    {
        DaemonLog(-1, "监听SIGCHLD信号失败");
        exit(1);
    }
    else
    {
        DaemonLog(0, "监听SIGCHLD信号成功");
    }

    /*设置SIGPIPE的处理函数为Plumber*/
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = Plumber;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGPIPE, &sa, NULL) < 0 )
    {
        DaemonLog(-1, "监听SIGPIPE信号失败");
        exit(1);
    }
    else
    {
        DaemonLog(0, "监听SIGPIPE信号成功");
    }

    /*设置SIGTERM的处理函数为Term*/
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = Term;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGTERM, &sa, NULL) < 0 )
    {
        DaemonLog(-1, "监听SIGTERM信号失败");
        exit(1);
    }
    else
    {
        DaemonLog(0, "监听SIGTERM信号成功");
    }
}

/*
 * 函数说明：读取配置文件
 * 参数1：本地ip
 * 参数2：本地port
 * 参数3：服务端ip
 * 参数4：服务端port
 * 返回值：无
 * */
void ReadConf(char *local_ip, char *local_port, char *remote_ip, char *remote_port)
{
    FILE *fp = NULL;
    int file_len = 0;
    char *file_buf = NULL;

    cJSON *json = NULL;
    cJSON *item = NULL;

    fp = fopen("keep_alive.conf", "rb");
    if(NULL == fp)
    {
        DaemonLog(-1, "打开配置文件失败");
        exit(1);
    }
    else
    {
        DaemonLog(0, "打开配置文件成功");
    }

    /*把文件指针移动到文件尾*/
    fseek(fp, 0, SEEK_END);
    /*获取文件内容长度*/
    file_len = ftell(fp);
    /*把文件指针移动到文件首*/
    fseek(fp, 0, SEEK_SET);

    file_buf = (char *)malloc((file_len + 1) * sizeof(char));
    if(NULL == file_buf)
    {
        DaemonLog(-1, "分配内存失败");
        fclose(fp);

        exit(1);
    }
    else
    {
        DaemonLog(0, "分配内存成功");
    }
    memset(file_buf, 0, file_len + 1);

    fread(file_buf, file_len, 1, fp);
    file_buf[file_len] = '\0';
    fclose(fp);

    /*载入JSON字符串*/
    json = cJSON_Parse(file_buf);
    if(NULL == json)
    {
        DaemonLog(-1, "载入JSON字符串失败");
        free(file_buf);
        file_buf = NULL;
        exit(1);
    }
    else
    {
        DaemonLog(0, "载入JSON字符串成功");
    }

    item = cJSON_GetObjectItem(json, "Remote_IP");
    if(NULL == item)
    {
        DaemonLog(-1, "解析Remote_IP失败");
        cJSON_Delete(json);
        json = NULL;
        free(file_buf);
        file_buf = NULL;
        exit(1);
    }
    else
    {
        DaemonLog(0, "解析Remote_IP成功");
    }
    memcpy(remote_ip, item->valuestring, strlen(item->valuestring));

    item = cJSON_GetObjectItem(json, "Remote_Port");
    if(NULL == item)
    {
        DaemonLog(-1, "解析Remote_Port失败");
        cJSON_Delete(json);
        json = NULL;
        free(file_buf);
        file_buf = NULL;
        exit(1);
    }
    else
    {
        DaemonLog(0, "解析Remote_Port成功");
    }
    memcpy(remote_port, item->valuestring, strlen(item->valuestring));

    item = cJSON_GetObjectItem(json, "Local_IP");
    if(NULL == item)
    {
        DaemonLog(-1, "解析Local_IP失败");
        cJSON_Delete(json);
        json = NULL;
        free(file_buf);
        file_buf = NULL;
        exit(1);
    }
    else
    {
        DaemonLog(0, "解析Local_IP成功");
    }
    memcpy(local_ip, item->valuestring, strlen(item->valuestring));

    item = cJSON_GetObjectItem(json, "Local_Port");
    if(NULL == item)
    {
        DaemonLog(-1, "解析Local_Port失败");
        cJSON_Delete(json);
        json = NULL;
        free(file_buf);
        file_buf = NULL;
        exit(1);
    }
    else
    {
        DaemonLog(0, "解析Local_Port成功");
    }
    memcpy(local_port, item->valuestring, strlen(item->valuestring));

    cJSON_Delete(json);
    json = NULL;
    free(file_buf);
    file_buf = NULL;
}

/*
 * 函数说明：将文件描述符设置成非阻塞
 * 参数1：文件描述符
 * 返回值：旧的文件描述符
 * */
int SetNonBlocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);

    return old_option;
}

/*
 * 函数说明：将文件描述符fd注册到epollfd所在的epoll内核事件中，
 * 参数1: epoll文件描述符
 * 参数2：待加入epoll的文件描述符fd
 * 返回值：无
 * */
void AddFd(int epoll_fd, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events =  EPOLLIN;

    event.events |= EPOLLET;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    SetNonBlocking(fd);
}

/*
 * 函数说明：删除epoll上注册的事件
 * 参数1: epoll文件描述符
 * 参数2：待从epoll中删除的文件描述符fd
 * 返回值：无
 * */
void RemoveFd(int epoll_fd, int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

/*
 * 函数说明：保持长连接
 * 参数：无
 * 返回值：无
 * */
void KeepAlive()
{
    int res;

    /*
     * 配置文件里的地址
     * 客户端IP：Local_IP
     * 客户端端口：Local_Port
     * 服务端IP：Remote_IP
     * 服务端Port：Remote_Port
     * */
    char server_ip[20] = {0};
    char server_port[10] = {0};
    char client_ip[20] = {0};
    char client_port[10] = {0};

    /*
     * 读取配置文件里的IP和端口
     * 中行的数据发送到server_ip,server_port
     * 联合电子的IP和端口client_ip,client_port
     * */
    ReadConf(server_ip, server_port, client_ip, client_port);

    DaemonLog(0, "服务端ip：");
    DaemonLog(0, server_ip);
    DaemonLog(0, "服务端port：");
    DaemonLog(0, server_port);
    DaemonLog(0, "联合电子ip：");
    DaemonLog(0, client_ip);
    DaemonLog(0, "联合电子port：");
    DaemonLog(0, client_port);

    /*创建server_address,作为服务端监听是否有数据过来*/
    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &server_address.sin_addr);
    server_address.sin_port = htons( atoi(server_port) );

    /*创建client_address,作为客户端用于连接联合电子的服务器*/
    struct sockaddr_in client_address;
    bzero(&client_address, sizeof(client_address));
    client_address.sin_family = AF_INET;
    inet_pton(AF_INET, client_ip, &client_address.sin_addr);
    client_address.sin_port = htons( atoi(client_port) );

    /*创建一个服务端监听server_ip,server_port*/
    /*建立socket*/
    g_server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(g_server_fd != -1)
    {
        DaemonLog(0, "建立服务端socket成功");
    }
    else
    {
        DaemonLog(-1, "建立服务端socket失败");

        exit(1);
    }

    int reuse = 1;
    /*设置复用端口，避免处于time_wait状态的端口不能使用*/
    setsockopt(g_server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    /*绑定socket*/
    res = bind(g_server_fd, (struct sockaddr *)&server_address, sizeof(server_address));
    if(res != -1)
    {
        DaemonLog(0, "绑定服务端socket成功");
    }
    else
    {
        DaemonLog(-1, "绑定服务端socket失败");
        close(g_server_fd);

        exit(1);
    }

    /*监听socket*/
    res = listen(g_server_fd, 5);
    if(res != -1)
    {
        DaemonLog(0, "监听服务端socket成功");
    }
    else
    {
        DaemonLog(-1, "监听服务端socket失败");
        close(g_server_fd);

        exit(1);
    }

    /*创建一个客户端socket连接client_ip,client_port*/
    /*建立socket*/
    g_client_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(g_client_fd != -1)
    {
        DaemonLog(0, "建立客户端socket成功");
    }
    else
    {
        DaemonLog(-1, "建立客户端socket失败");
        close(g_server_fd);

        exit(1);
    }
    res = connect(g_client_fd, (struct sockaddr *)&client_address, sizeof(client_address));
    if(res != -1)
    {
        DaemonLog(0, "连接联合电子服务器成功");
    }
    else
    {
        DaemonLog(-1, "连接联合电子服务器失败");
        close(g_server_fd);
        close(g_client_fd);

        exit(1);
    }

    /*发送过来的数据保存到这里*/
    char server_recv_buf[BUFFER_SIZE];
    /*联合电子发送过来的数据保存到这里*/
    char client_recv_buf[BUFFER_SIZE];

    struct epoll_event events[MAX_EVENT_NUMBER];

    g_epoll_fd = epoll_create(5);
    if(g_epoll_fd!= -1)
    {
        DaemonLog(0, "创建epoll成功");
    }
    else
    {
        DaemonLog(-1, "创建epoll失败");
        close(g_server_fd);
        close(g_client_fd);

        exit(1);
    }

    AddFd(g_epoll_fd, g_server_fd);
    AddFd(g_epoll_fd, g_client_fd);

    int conn_fd;
    int epoll_res;

    while(1)
    {
        epoll_res = epoll_wait(g_epoll_fd, events, MAX_EVENT_NUMBER, -1);
        if(epoll_res < 0)
        {
            DaemonLog(-1, "epoll_wait执行失败");
            continue;
        }
        for(int i = 0; i < epoll_res; ++i)
        {
            int sock_fd = events[i].data.fd;

            /*有连接过来*/
            if(sock_fd == g_server_fd)
            {
                /*定义客户端地址*/
                struct sockaddr_in client_address;
                socklen_t client_address_len = sizeof(client_address);

                /*接受客户端连接*/
                conn_fd = accept(g_server_fd, (struct sockaddr *)&client_address, &client_address_len);
                if(-1 == conn_fd)
                {
                    DaemonLog(-1, "服务端接受客户连接失败");
                    continue;
                }
                else
                {
                    DaemonLog(0, "服务端接受客户连接成功");
                }
                AddFd(g_epoll_fd, conn_fd);

                /*打印连接到的IP和port*/
                char client_address_ip[INET_ADDRSTRLEN + 1];
                memset(client_address_ip, 0, sizeof(client_address_ip));
                inet_ntop(AF_INET, &client_address.sin_addr, client_address_ip, INET_ADDRSTRLEN);
                DaemonLog(0, "连接到的客户端IP为：");
                DaemonLog(0, client_address_ip);
                unsigned int client_address_port = ntohs(client_address.sin_port);
                char port[10];
                memset(port, 0, sizeof(port));
                sprintf(port, "%d", client_address_port);
                DaemonLog(0, "连接到的客户端port为：");
                DaemonLog(0, port);
            }
            /*联合电子有数据过来*/
            else if(sock_fd == g_client_fd)
            {
                DaemonLog(0, "开始接收联合电子数据");
                memset(client_recv_buf, '\0', BUFFER_SIZE);
                int recv_res = 0;
                int recv_len = 0;
                /*ET模式下需要循环读取数据*/
                while(1)
                {
                    recv_res = recv(g_client_fd, client_recv_buf+recv_res, BUFFER_SIZE-recv_res-1, 0);
                    if(recv_res < 0)
                    {
                        /*对于非阻塞IO，下面的条件成立表示数据已经全部读取完毕，此后，epoll就能再次触发sock_fd上的EPOLLIN事件，以驱动下一次读操作*/
                        if( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
                        {
                            DaemonLog(0, "接收联合电子的数据完毕");

                            /*把接收到的数据转发到conn_fd*/
                            DaemonLog(0, "收到联合电子的数据：");
                            DaemonLog(0, client_recv_buf);
                            /*先判断是否是心跳包0000,是就回一个心跳，不是就转发数据到conn_fd*/
                            int strncmp_res = strncmp("0000", client_recv_buf, 4);
                            if(0 == strncmp_res)
                            {
                                DaemonLog(0, "收到一个心跳包");
                                /*是心跳包，返回一个心跳包0000*/
                                int send_res = send(g_client_fd, "0000", strlen("0000"), 0);
                                if( send_res <= 0 )
                                {
                                    DaemonLog(-1, "发送心跳包失败");
                                }
                                DaemonLog(0, "返回一个心跳包");
                            }
                            else
                            {
                                /*把数据发送到农行的客户端conn_fd*/
                                int send_res = send(conn_fd, client_recv_buf, recv_len, 0);
                                if( send_res <= 0 )
                                {
                                    DaemonLog(-1, "发送到农行数据出错");
                                }
                                else
                                {
                                    DaemonLog(0, "发送到农行数据成功");
                                }
                            }
                            break;
                        }
                        else
                        {
                            DaemonLog(0, "接收联合电子的数据出错");
                            break;
                        }
                    }
                    else if(0 == recv_res)
                    {
                        DaemonLog(-1, "联合电子关闭了连接，守护进程将在10秒后重新连接");
                        RemoveFd(g_epoll_fd, g_client_fd);

                        sleep(10);

                        g_client_fd = socket(PF_INET, SOCK_STREAM, 0);
                        if(-1 == g_client_fd)
                        {
                            DaemonLog(-1, "重新创建socket失败，守护进程退出");

                            if(g_server_fd != -1)
                            {
                                close(g_server_fd);
                                g_server_fd = -1;
                            }
                            if(g_client_fd != -1)
                            {
                                close(g_client_fd);
                                g_client_fd = -1;
                            }
                            if(g_file_fd != NULL)
                            {
                                fclose(g_file_fd);
                                g_file_fd = NULL;
                            }

                            exit(1);
                        }
                        else
                        {
                            DaemonLog(0, "重新创建socket成功");
                        }
                        int conn_res = connect(g_client_fd, (struct sockaddr *)&client_address, sizeof(client_address));
                        if(conn_res != -1)
                        {
                            DaemonLog(0, "重新连接联合电子服务器成功");
                            AddFd(g_epoll_fd, g_client_fd);
                        }
                        else
                        {
                            DaemonLog(-1, "重新连接联合电子服务器失败，守护进程退出");

                            if(g_server_fd != -1)
                            {
                                close(g_server_fd);
                                g_server_fd = -1;
                            }
                            if(g_client_fd != -1)
                            {
                                close(g_client_fd);
                                g_client_fd = -1;
                            }
                            if(g_file_fd != NULL)
                            {
                                fclose(g_file_fd);
                                g_file_fd = NULL;
                            }

                            exit(1);
                        }
                        break;
                    }
                    else
                    {
                        DaemonLog(0, "继续接收联合电子数据");

                        recv_len = recv_res;
                        continue;
                    }
                }/* end of while(1)*/
            }
            /*农行有数据过来*/
            else if(sock_fd == conn_fd)
            {
                DaemonLog(0, "开始接收农行数据");
                memset(server_recv_buf, '\0', BUFFER_SIZE);
                int recv_res = 0;
                int recv_len = 0;
                /*ET模式下需要循环读取数据*/
                while(1)
                {
                    recv_res = recv(conn_fd, server_recv_buf+recv_res, BUFFER_SIZE-recv_res-1, 0);
                    if(recv_res < 0)
                    {
                        /*对于非阻塞IO，下面的条件成立表示数据已经全部读取完毕，此后，epoll就能再次触发sock_fd上的EPOLLIN事件，以驱动下一次读操作*/
                        if( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
                        {
                            DaemonLog(0, "接收农行的数据完毕");

                            /*把数据发送到联合电子的g_client_fd*/
                            int len = recv_len / 2;
                            char len_str[10] = {'\0'};
                            sprintf(len_str, "%4d", len);
                            for(int i = 0; i < 4; ++i)
                            {
                                if(len_str[i] == ' ')
                                {
                                    len_str[i] = '0';
                                }
                            }
                            DaemonLog(0, "发送到联合电子数据的数据长度：");
                            DaemonLog(0, len_str);
                            int send_res = send(g_client_fd, len_str, strlen(len_str), 0);
                            if( send_res <= 0 )
                            {
                                DaemonLog(-1, "发送到联合电子数据长度出错");
                            }
                            else
                            {
                                DaemonLog(0, "发送到联合电子数据长度成功");
                            }
                            /*把接收到的数据转发到联合电子的g_client_fd*/
                            DaemonLog(0, "发送到联合电子数据的数据为：");
                            DaemonLog(0, server_recv_buf);

                            send_res = send(g_client_fd, server_recv_buf, strlen(server_recv_buf), 0);
                            if( send_res <= 0 )
                            {
                                DaemonLog(-1, "发送到联合电子数据出错");
                            }
                            else
                            {
                                DaemonLog(0, "发送到联合电子数据成功");
                            }
                            break;
                        }
                        else
                        {
                            DaemonLog(-1, "接收农行的数据出错");
                            break;
                        }
                    }
                    else if(0 == recv_res)
                    {
                        DaemonLog(-1, "农行关闭了连接");
                        /*关闭conn_fd*/
                        RemoveFd(g_epoll_fd, conn_fd);
                        break;
                    }
                    else
                    {
                        DaemonLog(0, "继续接收农行数据");
                        recv_len = recv_res;
                        continue;
                    }
                }
            }/*end of else if(sock_fd == conn_fd)*/
        }/*end of for(int i = 0; i < epoll_res; ++i) */
    }/* end of while(1)*/
}



