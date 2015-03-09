/**
 * reference:
 *   http://stackoverflow.com/questions/17954432/creating-a-daemon-in-linux
 *   http://www.linuxprofilm.com/articles/linux-daemon-howto.html
 */
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>

/**
 * FIXME
 * 1. customize signal handler and umask
 * 2. close log file
 */
static pid_t
daemonize(const char *wd, const char *log_ident)
{
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
	exit(EXIT_FAILURE);
    }
    /* If we are parent */
    if (pid > 0) {
	exit(EXIT_SUCCESS);
    }

    /* Child becomes the session leader */
    if (setsid() < 0) {
	exit(EXIT_FAILURE);
    }

    /* Handle signals, TODO implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time to give up session leader */
    pid = fork();
    if (pid < 0) {
	exit(EXIT_FAILURE);
    }
    if (pid > 0) {
	exit(EXIT_FAILURE);
    }

    /* Change the file mode mask */
    umask(0);

    /* Change the working directory to root or others */
    if (wd && (chdir(wd)) < 0) {
	exit(EXIT_FAILURE);
    }

    /* Close all open file descriptors */
    int i;
    for (i = sysconf(_SC_OPEN_MAX); i > 0; i--) {
        close(i);
    }

    /* Open the log file */
    if (log_ident) {
	openlog(log_ident, LOG_PID | LOG_CONS, LOG_DAEMON);
    }

    return pid;
}
