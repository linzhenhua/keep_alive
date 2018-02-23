#include "keep_alive.h"
#include "ytk_daemon_log.h"

int main(int argc, char **argv)
{
    /*ReadConfiguration(); */

    /*启动守护进程*/
    InitDaemon();

    /*保持长连接*/
    KeepAlive();

    return 0;
}
