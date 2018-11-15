#include <stdio.h>
#include <stdlib.h>

#include "ping.h"

#define DEFULT_TEST_HOST "www.baidu.com"

int main(int argc, char * argv[])
{
    int ret = -1;
    char * host = DEFULT_TEST_HOST;
    switch(argc)
    {
        case 1:
            break;
        case 2:
            host = argv[1];
        default:
            break;
    }

    ret = ping_host_ip(host);
    if (ret < 0)
    {
        printf("Net is offline!\n");
    }
    else
    {
        printf("Net is online!\n");
    }

    return 0;
}
