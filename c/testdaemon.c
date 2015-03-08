#include <stdio.h>
#include "daemonize.h"

int main() {
    daemonize(".", "testdaemon");
    syslog (LOG_NOTICE, "First daemon started.");
    sleep(5);
    syslog (LOG_NOTICE, "First daemon terminated.");
    closelog();

    return 0;
}
