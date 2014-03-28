#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "ktoy.h"

int main(int argc, char *argv[])
{
    int fd, ret;
    volatile int ktoy_val = 0;

    fd = open("/dev/ktoy", O_RDWR);
    if (fd == -1) {
        perror("open /dev/ktoy");
        exit(1);
    }

    ret = ioctl(fd, KTOY_IOC_GET, &ktoy_val);

    if (ret == -1) {
        perror("ioctl KTOY_IOC_GET");
        exit(1);
    }
    printf("ktoy_val: %8x\n", ktoy_val);

    close(fd);
        
    return 0;
}
