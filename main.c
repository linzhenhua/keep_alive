#include "daemon.h"

int main(int argc, char **argv)
{
    /*开启守护进程*/
    InitDaemon();

    KeepAlive();

    return 0;
}
