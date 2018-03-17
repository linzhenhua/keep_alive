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
 * ����˵������¼�ػ�����ִ�й���
 * ����1��������
 * ����2��������Ϣ
 * ����3���ļ���
 * ����4���к�
 * ����5��������
 * ����ֵ����
 * */
void _DaemonLog(const int error_code, const char *error_msg, const char *file_name, int line, const char *func)
{
    struct timeval tv;
    struct tm *now_time;

    memset(&tv, 0, sizeof(struct timeval));

    gettimeofday(&tv, NULL);

    now_time = localtime(&tv.tv_sec);

    char log_name[30] = {'\0'};

    //��־����
    sprintf(log_name, "daemon_%04d%02d%02d.log", now_time->tm_year+1900, now_time->tm_mon+1, now_time->tm_mday);

    //���ļ�
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
 * ����˵�����ض������ļ�
 * ��������
 * ����ֵ����
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
        DaemonLog(-1, "�������ļ�ʧ��");
    }
    else
    {
        DaemonLog(0, "�������ļ��ɹ�");
    }

    /*���ļ�ָ���ƶ����ļ�β*/
    fseek(fp, 0, SEEK_END);
    /*��ȡ�ļ����ݳ���*/
    file_len = ftell(fp);
    /*���ļ�ָ���ƶ����ļ���*/
    fseek(fp, 0, SEEK_SET);

    pszFileBuf = (char *)malloc((file_len + 1) * sizeof(char));
    if(NULL == pszFileBuf)
    {
        DaemonLog(-1, "�����ڴ�ʧ��");
        fclose(fp);

        exit(1);
    }
    else
    {
        DaemonLog(0, "�����ڴ�ɹ�");
    }
    memset(pszFileBuf, 0, file_len + 1);

    fread(pszFileBuf, file_len, 1, fp);
    pszFileBuf[file_len] = '\0';
    fclose(fp);

    /*����JSON�ַ���*/
    pszJson = cJSON_Parse(pszFileBuf);
    if(NULL == pszJson)
    {
        DaemonLog(-1, "����JSON�ַ���ʧ��");
        free(pszFileBuf);
        pszFileBuf = NULL;
    }
    else
    {
        DaemonLog(0, "����JSON�ַ����ɹ�");
    }

    pszItem = cJSON_GetObjectItem(pszJson, "Remote_IP");
    if(NULL == pszItem)
    {
        DaemonLog(-1, "����Remote_IPʧ��");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
    }
    else
    {
        DaemonLog(0, "����Remote_IP�ɹ�");
    }
    memcpy(pszRemoteIp, pszItem->valuestring, strlen(pszItem->valuestring));

    pszItem = cJSON_GetObjectItem(pszJson, "Remote_Port");
    if(NULL == pszItem)
    {
        DaemonLog(-1, "����Remote_Portʧ��");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
    }
    else
    {
        DaemonLog(0, "����Remote_Port�ɹ�");
    }
    memcpy(pszRemotePort, pszItem->valuestring, strlen(pszItem->valuestring));

    pszItem = cJSON_GetObjectItem(pszJson, "Local_IP");
    if(NULL == pszItem)
    {
        DaemonLog(-1, "����Local_IPʧ��");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
    }
    else
    {
        DaemonLog(0, "����Local_IP�ɹ�");
    }
    memcpy(pszLocalIp, pszItem->valuestring, strlen(pszItem->valuestring));

    pszItem = cJSON_GetObjectItem(pszJson, "Local_Port");
    if(NULL == pszItem)
    {
        DaemonLog(-1, "����Local_Portʧ��");
        cJSON_Delete(pszJson);
        pszJson = NULL;
        free(pszFileBuf);
        pszFileBuf = NULL;
    }
    else
    {
        DaemonLog(0, "����Local_Port�ɹ�");
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

    DaemonLog(0, "���¶�ȡ�����ļ��ɹ�");
}

/*
 * ����˵����SIGHUP���������ض������ļ���
 * ������δʹ��
 * ����ֵ����
 * */
void Hup(int s)
{
    DaemonLog(0, "�������¶�ȡ�����ļ�");

    struct sigaction sa;

    /*Learn the new rules*/
    ReReadConfiguration();

    /*And reinstall the signal handler*/
    sa.sa_handler = Hup;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGHUP, &sa, NULL) < 0)
    {
        DaemonLog(-1, "���¼���SIGHUP�ź�ʧ��");
    }
    else
    {
        DaemonLog(0, "���¼���SIGHUP�źųɹ�");
    }
}

/*
 * ����˵����SIGPIPE�����������رյ�socket�ļ�������д���߶����ݴ���SIGPIPE�źţ�
 * ������δʹ��
 * ����ֵ����
 * */
void Plumber(int s)
{
    DaemonLog(0, "���رյ�socket�ļ�������������д����");

    struct sigaction sa;

    /*just reinstall*/
    sa.sa_handler = Plumber;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGPIPE, &sa, NULL) < 0)
    {
        DaemonLog(-1, "���¼���SIGPIPE�ź�ʧ��");
    }
    else
    {
        DaemonLog(0, "���¼���SIGPIPE�źųɹ�");
    }
}

/*
 * ����˵����SIGTERM����������kill�ػ�����ʱ�������ͷ�������Դ
 * ������δʹ��
 * ����ֵ����
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

    DaemonLog(0, "���̽���\n");
    exit(0);
}

/*
 * ��̹���
 * 1������Ҫ�����ǵ���umask���ļ�ģʽ��������������Ϊ0
 * 2������fork��Ȼ��ʹ�������˳���exit��
 * 3������setsid�Դ���һ���»Ự
 * 4������ǰ����Ŀ¼����Ϊ��Ŀ¼
 * 5���رղ�����Ҫ���ļ�������
 * 6��ĳЩ�ػ����̴�/dev/nullʹ������ļ�������0,1,2�������κ�һ����ͼ����׼���롢
 * д��׼������׼����Ŀ����̶���������κ�Ч��
 *
 * ����˵��������һ���ػ�����
 * ������δʹ��
 * ����ֵ����
 * */
void InitDaemon()
{
    int i, fd;
    pid_t pid;
    struct sigaction sa;

    /*תΪ��̨����*/
    if( (pid = fork()) < 0 )
    {
        DaemonLog(-1, "�����ػ�����ʧ��");
        exit(1);
    }
    else if(0 != pid)
    {
        DaemonLog(0, "�������˳�");
        exit(0);
    }

    /*�����µĻỰ�飬��Ϊ�Ự�鳤�ͽ����鳤*/
    setsid();

    /*ʹ�䲻���ǻỰ�鳤�����ܿ����ն�*/
    if( (pid = fork()) < 0 )
    {
        DaemonLog(-1, "�����ػ�����ʧ��");
        exit(1);
    }
    else if(0 != pid)
    {
        DaemonLog(0, "�������˳�");
        exit(0);
    }

    /*�ر��Ѿ��򿪵��ļ��������������˷�ϵͳ��Դ*/
    for(i = 0; i < NOFILE; ++i)
    {
        close(i);
    }

    /*���Ĺ���Ŀ¼*/
    int res;
    res = chdir(DIR);
    if( res < 0 )
    {
        DaemonLog(-1, "���Ĺ���Ŀ¼ʧ��");
        exit(1);
    }
    else
    {
        DaemonLog(0, "���Ĺ���Ŀ¼�ɹ�");
    }

    /*�����ļ����룬ʹ�ļ�����Ȩ�޲����ܸ�����Ӱ��*/
    umask(0);

    /*�ض����������*/
    fd = open("/dev/null", O_RDWR);
    if(fd < 0)
    {
        DaemonLog(-1, "�ض����������ʧ��");
        exit(1);
    }
    else
    {
        DaemonLog(0, "�ض�����������ɹ�");
    }

    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);

    /*����SIGHUP�Ĵ�����ΪHup���ض������ļ�*/
    sa.sa_handler = Hup;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGHUP, &sa, NULL) < 0 )
    {
        DaemonLog(-1, "����SIGHUP�ź�ʧ��");
        exit(1);
    }
    else
    {
        DaemonLog(0, "����SIGHUP�źųɹ�");
    }

    /*����SIGCHLD�źţ����������ʬ����ռ��ϵͳ��Դ*/
    /*����SIGCHLD�Ĵ�����ΪSIG_ING*/
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGCHLD, &sa, NULL) < 0 )
    {
        DaemonLog(-1, "����SIGCHLD�ź�ʧ��");
        exit(1);
    }
    else
    {
        DaemonLog(0, "����SIGCHLD�źųɹ�");
    }

    /*����SIGPIPE�Ĵ�����ΪPlumber*/
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = Plumber;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGPIPE, &sa, NULL) < 0 )
    {
        DaemonLog(-1, "����SIGPIPE�ź�ʧ��");
        exit(1);
    }
    else
    {
        DaemonLog(0, "����SIGPIPE�źųɹ�");
    }

    /*����SIGTERM�Ĵ�����ΪTerm*/
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = Term;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if( sigaction(SIGTERM, &sa, NULL) < 0 )
    {
        DaemonLog(-1, "����SIGTERM�ź�ʧ��");
        exit(1);
    }
    else
    {
        DaemonLog(0, "����SIGTERM�źųɹ�");
    }
}

/*
 * ����˵������ȡ�����ļ�
 * ����1������ip
 * ����2������port
 * ����3�������ip
 * ����4�������port
 * ����ֵ����
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
        DaemonLog(-1, "�������ļ�ʧ��");
        exit(1);
    }
    else
    {
        DaemonLog(0, "�������ļ��ɹ�");
    }

    /*���ļ�ָ���ƶ����ļ�β*/
    fseek(fp, 0, SEEK_END);
    /*��ȡ�ļ����ݳ���*/
    file_len = ftell(fp);
    /*���ļ�ָ���ƶ����ļ���*/
    fseek(fp, 0, SEEK_SET);

    file_buf = (char *)malloc((file_len + 1) * sizeof(char));
    if(NULL == file_buf)
    {
        DaemonLog(-1, "�����ڴ�ʧ��");
        fclose(fp);

        exit(1);
    }
    else
    {
        DaemonLog(0, "�����ڴ�ɹ�");
    }
    memset(file_buf, 0, file_len + 1);

    fread(file_buf, file_len, 1, fp);
    file_buf[file_len] = '\0';
    fclose(fp);

    /*����JSON�ַ���*/
    json = cJSON_Parse(file_buf);
    if(NULL == json)
    {
        DaemonLog(-1, "����JSON�ַ���ʧ��");
        free(file_buf);
        file_buf = NULL;
        exit(1);
    }
    else
    {
        DaemonLog(0, "����JSON�ַ����ɹ�");
    }

    item = cJSON_GetObjectItem(json, "Remote_IP");
    if(NULL == item)
    {
        DaemonLog(-1, "����Remote_IPʧ��");
        cJSON_Delete(json);
        json = NULL;
        free(file_buf);
        file_buf = NULL;
        exit(1);
    }
    else
    {
        DaemonLog(0, "����Remote_IP�ɹ�");
    }
    memcpy(remote_ip, item->valuestring, strlen(item->valuestring));

    item = cJSON_GetObjectItem(json, "Remote_Port");
    if(NULL == item)
    {
        DaemonLog(-1, "����Remote_Portʧ��");
        cJSON_Delete(json);
        json = NULL;
        free(file_buf);
        file_buf = NULL;
        exit(1);
    }
    else
    {
        DaemonLog(0, "����Remote_Port�ɹ�");
    }
    memcpy(remote_port, item->valuestring, strlen(item->valuestring));

    item = cJSON_GetObjectItem(json, "Local_IP");
    if(NULL == item)
    {
        DaemonLog(-1, "����Local_IPʧ��");
        cJSON_Delete(json);
        json = NULL;
        free(file_buf);
        file_buf = NULL;
        exit(1);
    }
    else
    {
        DaemonLog(0, "����Local_IP�ɹ�");
    }
    memcpy(local_ip, item->valuestring, strlen(item->valuestring));

    item = cJSON_GetObjectItem(json, "Local_Port");
    if(NULL == item)
    {
        DaemonLog(-1, "����Local_Portʧ��");
        cJSON_Delete(json);
        json = NULL;
        free(file_buf);
        file_buf = NULL;
        exit(1);
    }
    else
    {
        DaemonLog(0, "����Local_Port�ɹ�");
    }
    memcpy(local_port, item->valuestring, strlen(item->valuestring));

    cJSON_Delete(json);
    json = NULL;
    free(file_buf);
    file_buf = NULL;
}

/*
 * ����˵�������ļ����������óɷ�����
 * ����1���ļ�������
 * ����ֵ���ɵ��ļ�������
 * */
int SetNonBlocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);

    return old_option;
}

/*
 * ����˵�������ļ�������fdע�ᵽepollfd���ڵ�epoll�ں��¼��У�
 * ����1: epoll�ļ�������
 * ����2��������epoll���ļ�������fd
 * ����ֵ����
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
 * ����˵����ɾ��epoll��ע����¼�
 * ����1: epoll�ļ�������
 * ����2������epoll��ɾ�����ļ�������fd
 * ����ֵ����
 * */
void RemoveFd(int epoll_fd, int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

/*
 * ����˵�������ֳ�����
 * ��������
 * ����ֵ����
 * */
void KeepAlive()
{
    int res;

    /*
     * �����ļ���ĵ�ַ
     * �ͻ���IP��Local_IP
     * �ͻ��˶˿ڣ�Local_Port
     * �����IP��Remote_IP
     * �����Port��Remote_Port
     * */
    char server_ip[20] = {0};
    char server_port[10] = {0};
    char client_ip[20] = {0};
    char client_port[10] = {0};

    /*
     * ��ȡ�����ļ����IP�Ͷ˿�
     * ���е����ݷ��͵�server_ip,server_port
     * ���ϵ��ӵ�IP�Ͷ˿�client_ip,client_port
     * */
    ReadConf(server_ip, server_port, client_ip, client_port);

    DaemonLog(0, "�����ip��");
    DaemonLog(0, server_ip);
    DaemonLog(0, "�����port��");
    DaemonLog(0, server_port);
    DaemonLog(0, "���ϵ���ip��");
    DaemonLog(0, client_ip);
    DaemonLog(0, "���ϵ���port��");
    DaemonLog(0, client_port);

    /*����server_address,��Ϊ����˼����Ƿ������ݹ���*/
    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &server_address.sin_addr);
    server_address.sin_port = htons( atoi(server_port) );

    /*����client_address,��Ϊ�ͻ��������������ϵ��ӵķ�����*/
    struct sockaddr_in client_address;
    bzero(&client_address, sizeof(client_address));
    client_address.sin_family = AF_INET;
    inet_pton(AF_INET, client_ip, &client_address.sin_addr);
    client_address.sin_port = htons( atoi(client_port) );

    /*����һ������˼���server_ip,server_port*/
    /*����socket*/
    g_server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(g_server_fd != -1)
    {
        DaemonLog(0, "���������socket�ɹ�");
    }
    else
    {
        DaemonLog(-1, "���������socketʧ��");

        exit(1);
    }

    int reuse = 1;
    /*���ø��ö˿ڣ����⴦��time_wait״̬�Ķ˿ڲ���ʹ��*/
    setsockopt(g_server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    /*��socket*/
    res = bind(g_server_fd, (struct sockaddr *)&server_address, sizeof(server_address));
    if(res != -1)
    {
        DaemonLog(0, "�󶨷����socket�ɹ�");
    }
    else
    {
        DaemonLog(-1, "�󶨷����socketʧ��");
        close(g_server_fd);

        exit(1);
    }

    /*����socket*/
    res = listen(g_server_fd, 5);
    if(res != -1)
    {
        DaemonLog(0, "���������socket�ɹ�");
    }
    else
    {
        DaemonLog(-1, "���������socketʧ��");
        close(g_server_fd);

        exit(1);
    }

    /*����һ���ͻ���socket����client_ip,client_port*/
    /*����socket*/
    g_client_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(g_client_fd != -1)
    {
        DaemonLog(0, "�����ͻ���socket�ɹ�");
    }
    else
    {
        DaemonLog(-1, "�����ͻ���socketʧ��");
        close(g_server_fd);

        exit(1);
    }
    res = connect(g_client_fd, (struct sockaddr *)&client_address, sizeof(client_address));
    if(res != -1)
    {
        DaemonLog(0, "�������ϵ��ӷ������ɹ�");
    }
    else
    {
        DaemonLog(-1, "�������ϵ��ӷ�����ʧ��");
        close(g_server_fd);
        close(g_client_fd);

        exit(1);
    }

    /*���͹��������ݱ��浽����*/
    char server_recv_buf[BUFFER_SIZE];
    /*���ϵ��ӷ��͹��������ݱ��浽����*/
    char client_recv_buf[BUFFER_SIZE];

    struct epoll_event events[MAX_EVENT_NUMBER];

    g_epoll_fd = epoll_create(5);
    if(g_epoll_fd!= -1)
    {
        DaemonLog(0, "����epoll�ɹ�");
    }
    else
    {
        DaemonLog(-1, "����epollʧ��");
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
            DaemonLog(-1, "epoll_waitִ��ʧ��");
            continue;
        }
        for(int i = 0; i < epoll_res; ++i)
        {
            int sock_fd = events[i].data.fd;

            /*�����ӹ���*/
            if(sock_fd == g_server_fd)
            {
                /*����ͻ��˵�ַ*/
                struct sockaddr_in client_address;
                socklen_t client_address_len = sizeof(client_address);

                /*���ܿͻ�������*/
                conn_fd = accept(g_server_fd, (struct sockaddr *)&client_address, &client_address_len);
                if(-1 == conn_fd)
                {
                    DaemonLog(-1, "����˽��ܿͻ�����ʧ��");
                    continue;
                }
                else
                {
                    DaemonLog(0, "����˽��ܿͻ����ӳɹ�");
                }
                AddFd(g_epoll_fd, conn_fd);

                /*��ӡ���ӵ���IP��port*/
                char client_address_ip[INET_ADDRSTRLEN + 1];
                memset(client_address_ip, 0, sizeof(client_address_ip));
                inet_ntop(AF_INET, &client_address.sin_addr, client_address_ip, INET_ADDRSTRLEN);
                DaemonLog(0, "���ӵ��Ŀͻ���IPΪ��");
                DaemonLog(0, client_address_ip);
                unsigned int client_address_port = ntohs(client_address.sin_port);
                char port[10];
                memset(port, 0, sizeof(port));
                sprintf(port, "%d", client_address_port);
                DaemonLog(0, "���ӵ��Ŀͻ���portΪ��");
                DaemonLog(0, port);
            }
            /*���ϵ��������ݹ���*/
            else if(sock_fd == g_client_fd)
            {
                DaemonLog(0, "��ʼ�������ϵ�������");
                memset(client_recv_buf, '\0', BUFFER_SIZE);
                int recv_res = 0;
                int recv_len = 0;
                /*ETģʽ����Ҫѭ����ȡ����*/
                while(1)
                {
                    recv_res = recv(g_client_fd, client_recv_buf+recv_res, BUFFER_SIZE-recv_res-1, 0);
                    if(recv_res < 0)
                    {
                        /*���ڷ�����IO�����������������ʾ�����Ѿ�ȫ����ȡ��ϣ��˺�epoll�����ٴδ���sock_fd�ϵ�EPOLLIN�¼�����������һ�ζ�����*/
                        if( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
                        {
                            DaemonLog(0, "�������ϵ��ӵ��������");

                            /*�ѽ��յ�������ת����conn_fd*/
                            DaemonLog(0, "�յ����ϵ��ӵ����ݣ�");
                            DaemonLog(0, client_recv_buf);
                            /*���ж��Ƿ���������0000,�Ǿͻ�һ�����������Ǿ�ת�����ݵ�conn_fd*/
                            int strncmp_res = strncmp("0000", client_recv_buf, 4);
                            if(0 == strncmp_res)
                            {
                                DaemonLog(0, "�յ�һ��������");
                                /*��������������һ��������0000*/
                                int send_res = send(g_client_fd, "0000", strlen("0000"), 0);
                                if( send_res <= 0 )
                                {
                                    DaemonLog(-1, "����������ʧ��");
                                }
                                DaemonLog(0, "����һ��������");
                            }
                            else
                            {
                                /*�����ݷ��͵�ũ�еĿͻ���conn_fd*/
                                int send_res = send(conn_fd, client_recv_buf, recv_len, 0);
                                if( send_res <= 0 )
                                {
                                    DaemonLog(-1, "���͵�ũ�����ݳ���");
                                }
                                else
                                {
                                    DaemonLog(0, "���͵�ũ�����ݳɹ�");
                                }
                            }
                            break;
                        }
                        else
                        {
                            DaemonLog(0, "�������ϵ��ӵ����ݳ���");
                            break;
                        }
                    }
                    else if(0 == recv_res)
                    {
                        DaemonLog(-1, "���ϵ��ӹر������ӣ��ػ����̽���10�����������");
                        RemoveFd(g_epoll_fd, g_client_fd);

                        sleep(10);

                        g_client_fd = socket(PF_INET, SOCK_STREAM, 0);
                        if(-1 == g_client_fd)
                        {
                            DaemonLog(-1, "���´���socketʧ�ܣ��ػ������˳�");

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
                            DaemonLog(0, "���´���socket�ɹ�");
                        }
                        int conn_res = connect(g_client_fd, (struct sockaddr *)&client_address, sizeof(client_address));
                        if(conn_res != -1)
                        {
                            DaemonLog(0, "�����������ϵ��ӷ������ɹ�");
                            AddFd(g_epoll_fd, g_client_fd);
                        }
                        else
                        {
                            DaemonLog(-1, "�����������ϵ��ӷ�����ʧ�ܣ��ػ������˳�");

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
                        DaemonLog(0, "�����������ϵ�������");

                        recv_len = recv_res;
                        continue;
                    }
                }/* end of while(1)*/
            }
            /*ũ�������ݹ���*/
            else if(sock_fd == conn_fd)
            {
                DaemonLog(0, "��ʼ����ũ������");
                memset(server_recv_buf, '\0', BUFFER_SIZE);
                int recv_res = 0;
                int recv_len = 0;
                /*ETģʽ����Ҫѭ����ȡ����*/
                while(1)
                {
                    recv_res = recv(conn_fd, server_recv_buf+recv_res, BUFFER_SIZE-recv_res-1, 0);
                    if(recv_res < 0)
                    {
                        /*���ڷ�����IO�����������������ʾ�����Ѿ�ȫ����ȡ��ϣ��˺�epoll�����ٴδ���sock_fd�ϵ�EPOLLIN�¼�����������һ�ζ�����*/
                        if( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
                        {
                            DaemonLog(0, "����ũ�е��������");

                            /*�����ݷ��͵����ϵ��ӵ�g_client_fd*/
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
                            DaemonLog(0, "���͵����ϵ������ݵ����ݳ��ȣ�");
                            DaemonLog(0, len_str);
                            int send_res = send(g_client_fd, len_str, strlen(len_str), 0);
                            if( send_res <= 0 )
                            {
                                DaemonLog(-1, "���͵����ϵ������ݳ��ȳ���");
                            }
                            else
                            {
                                DaemonLog(0, "���͵����ϵ������ݳ��ȳɹ�");
                            }
                            /*�ѽ��յ�������ת�������ϵ��ӵ�g_client_fd*/
                            DaemonLog(0, "���͵����ϵ������ݵ�����Ϊ��");
                            DaemonLog(0, server_recv_buf);

                            send_res = send(g_client_fd, server_recv_buf, strlen(server_recv_buf), 0);
                            if( send_res <= 0 )
                            {
                                DaemonLog(-1, "���͵����ϵ������ݳ���");
                            }
                            else
                            {
                                DaemonLog(0, "���͵����ϵ������ݳɹ�");
                            }
                            break;
                        }
                        else
                        {
                            DaemonLog(-1, "����ũ�е����ݳ���");
                            break;
                        }
                    }
                    else if(0 == recv_res)
                    {
                        DaemonLog(-1, "ũ�йر�������");
                        /*�ر�conn_fd*/
                        RemoveFd(g_epoll_fd, conn_fd);
                        break;
                    }
                    else
                    {
                        DaemonLog(0, "��������ũ������");
                        recv_len = recv_res;
                        continue;
                    }
                }
            }/*end of else if(sock_fd == conn_fd)*/
        }/*end of for(int i = 0; i < epoll_res; ++i) */
    }/* end of while(1)*/
}



