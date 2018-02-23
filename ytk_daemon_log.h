#ifndef YTK_DAEMON_LOG
#define YTK_DAEMON_LOG

/*记录日志基本要求：时间，错误文件，错误行号，错误函数名，错误码，错误信息*/
/*
 * 参数1：错误码
 * 参数2：错误信息
 * 返回值：无
 * */
void _ytk_daemon_log(const int error_code, const char *error, 
        const char *file_name, int line, const char *func);

#define ytk_daemon_log(error_code, error) \
    _ytk_daemon_log(error_code, error, __FILE__, __LINE__, __FUNCTION__)

#endif
