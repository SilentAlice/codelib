#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>  /* for bzero */
#include <signal.h>
#include <aio.h>
#include <pthread.h>  /* to print pthread_self */

int fin = 0;
int fd;
struct aiocb aio_file;

void finish_aio(union sigval val) {
    printf("T%x aio finished\n", pthread_self());
    fin = 1;
    aio_return(&aio_file);
    close(fd);
}

void sigusr1_handler(int arg) {
    printf("T%x aio finished\n", pthread_self());
    fin = 1;
    aio_return(&aio_file);
    close(fd);
}

int main(int argc, char *argv[])
{
    char buf[4096];
    bzero(buf, sizeof buf);
    bzero(&aio_file, sizeof(struct aiocb));

    printf("T%x open file\n", pthread_self());
    fd = open("/dev/null", O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    /* int flags = fcntl(fd, F_GETFL, 0); */
    /* if (flags == -1) { */
    /*     perror("fcntl get flags"); */
    /*     close(fd); */
    /*     exit(1); */
    /* } */
    /* flags |= O_NONBLOCK; */
    /* if (fcntl(fd, F_SETFL, flags) < 0) { */
    /*     perror("fcntl set flags"); */
    /*     close(fd); */
    /*     exit(1); */
    /* } */

    aio_file.aio_fildes = fd;
    aio_file.aio_offset = 0;
    aio_file.aio_buf = buf;
    aio_file.aio_nbytes = sizeof buf;
#ifdef __MACH__
    aio_file.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    aio_file.aio_sigevent.sigev_signo = SIGUSR1;

    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGUSR1, &sa, NULL);
#elif __linux__
    aio_file.aio_sigevent.sigev_notify = SIGEV_THREAD;
    aio_file.aio_sigevent.sigev_notify_function = finish_aio;
#endif

    aio_file.aio_sigevent.sigev_notify_attributes = NULL;
    aio_file.aio_sigevent.sigev_value.sival_ptr = &aio_file;

    /* aio_cancel(fd, NULL); */
    int ret = aio_read(&aio_file);
    if (ret == -1) {
        perror("aio_read fail");
        aio_return(&aio_file);
        close(fd);
        exit(1);
    }

    while (!fin) {
        asm("pause\t\n":::"memory");
    }
    /* sleep(1); */
    /* int r = aio_return(&aio_file); */
    /* if (r == -1) { */
    /*     perror("aoi_return"); */
    /*     close(fd); */
    /*     exit(1); */
    /* } */

    /* close(fd); */
    return 0;
}
