#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*记录日志基本要求：时间，错误文件，错误行号，错误函数名，错误码，错误信息*/
void _ytk_daemon_log(const int ulErrorCode, const char *error, 
        const char *file_name, int line, const char *func)
{
    FILE *pszFile;
    
    char time_str[40] = {0};
    time_t t;
    struct tm *nowtime;
    
    char log_name[50];

    time(&t);
    nowtime = localtime(&t);
	
    sprintf(log_name, "ytk_daemon_%04d%02d%02d.log", nowtime->tm_year+1900, nowtime->tm_mon+1, nowtime->tm_mday);
    
    pszFile = fopen(log_name, "a");
    if(NULL == pszFile)
    {
        exit(1);
    }

    strftime(time_str, sizeof(time_str), "%Y-%m-%02d-%H:%M:%S", nowtime);
	
    fprintf(pszFile, "%s_%s_%d_%s_%d : %s\n", time_str, file_name, line, func, ulErrorCode, error);

    fclose(pszFile); 
}
