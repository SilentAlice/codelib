# snippets for C

## cl_aio.c
This is an demo for `aio_read`.

For OSX, I only managed to make it work by register signal handler `SIGUSR1`. I can't figure out why the new-thread-approach doesn't work. It reports `aio_read fail: Resource temporarily unavailable` on OSX.

On Linux, glibc will create a new thread to get blocked at the handler routine. It can be observed from output.

To compile this file

    # OSX
    gcc cl_aio.c -o cl_aio [-pthread]
    
    # Linux
    gcc cl_aio.c -o cl_aio -rt [-pthread]
