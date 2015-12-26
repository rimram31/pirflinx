/*
 * pirflinx
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h> 
#include <execinfo.h>
#include "Logger.h"

#define DAEMON_NAME "pirflinx"
#define PID_FILE "/var/run/pirflinx.pid" 

std::string szStartupFolder;
CLogger _log;
std::string logfile = "";
int socketPort = 9999;

bool g_bStopApplication = false;
bool g_bRunAsDaemon = false;
int pidFilehandle = 0;

int fatal_handling = 0;

int g_RepeatDefault = 1;
long g_DelayDefault = 5000L;

void signal_handler(int sig_num) {
	switch(sig_num) {
	case SIGHUP:
		if (logfile!="") {
			_log.SetOutputFile(logfile.c_str());
		}
		break;
	case SIGINT:
	case SIGTERM:
		g_bStopApplication = true;
		break;
	case SIGSEGV:
	case SIGILL:
	case SIGABRT:
	case SIGFPE:
		if (fatal_handling) {
			exit(EXIT_FAILURE);
		}
		fatal_handling = 1;
		// re-raise signal to enforce core dump
		signal(sig_num, SIG_DFL);
		raise(sig_num);
		break;
	} 
}

void daemonShutdown() {
	if (pidFilehandle != 0) {
		close(pidFilehandle);
		pidFilehandle = 0;
	}
}

void daemonize(const char *rundir, const char *pidfile) {
	int pid, sid, i;
	char str[10];
	struct sigaction newSigAction;
	sigset_t newSigSet;

	/* Check if parent process id is set */
	if (getppid() == 1) {
		/* PPID exists, therefore we are already a daemon */
		return;
	}

	/* Set signal mask - signals we want to block */
	sigemptyset(&newSigSet);
	sigaddset(&newSigSet, SIGCHLD);  /* ignore child - i.e. we don't need to wait for it */
	sigaddset(&newSigSet, SIGTSTP);  /* ignore Tty stop signals */
	sigaddset(&newSigSet, SIGTTOU);  /* ignore Tty background writes */
	sigaddset(&newSigSet, SIGTTIN);  /* ignore Tty background reads */
	sigprocmask(SIG_BLOCK, &newSigSet, NULL);   /* Block the above specified signals */

	/* Set up a signal handler */
	newSigAction.sa_handler = signal_handler;
	sigemptyset(&newSigAction.sa_mask);
	newSigAction.sa_flags = 0;

	/* Signals to handle */
	sigaction(SIGTERM, &newSigAction, NULL);    // catch term signal
	sigaction(SIGINT,  &newSigAction, NULL);    // catch interrupt signal
	sigaction(SIGSEGV, &newSigAction, NULL);    // catch segmentation fault signal
	sigaction(SIGABRT, &newSigAction, NULL);    // catch abnormal termination signal
	sigaction(SIGILL,  &newSigAction, NULL);    // catch invalid program image
	sigaction(SIGHUP,  &newSigAction, NULL);    // catch HUP, for logrotation
	
	/* Fork*/
	pid = fork();

	if (pid < 0) {
		/* Could not fork */
		exit(EXIT_FAILURE);
	}

	if (pid > 0) {
		/* Child created ok, so exit parent process */
		exit(EXIT_SUCCESS);
	}

	/* Ensure only one copy */
	pidFilehandle = open(pidfile, O_RDWR | O_CREAT, 0600);

	if (pidFilehandle == -1) {
		/* Couldn't open lock file */
		syslog(LOG_INFO, "Could not open PID lock file %s, exiting", pidfile);
		exit(EXIT_FAILURE);
	}

	/* Try to lock file */
	if (lockf(pidFilehandle, F_TLOCK, 0) == -1) {
		/* Couldn't get lock on lock file */
		syslog(LOG_INFO, "Could not lock PID lock file %s, exiting", pidfile);
		exit(EXIT_FAILURE);
	}

	/* Get and format PID */
	sprintf(str, "%d\n", getpid());

	/* write pid to lockfile */
	write(pidFilehandle, str, strlen(str));


	/* Child continues */
	umask(027); /* Set file permissions 750 */

	if (logfile!="") {
		_log.SetOutputFile(logfile.c_str());
	}

	/* Get a new process group */
	sid = setsid();

	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	/* Close out the standard file descriptors */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	/* Route I/O connections */

	/* Open STDIN */
	i = open("/dev/null", O_RDWR);

	/* STDOUT */
	dup(i);

	/* STDERR */
	dup(i);

	chdir(rundir); /* change running directory */
} 

static size_t getExecutablePathName(char* pathName, size_t pathNameCapacity) {
    size_t pathNameSize = readlink("/proc/self/exe", pathName, pathNameCapacity - 1);
    pathName[pathNameSize] = '\0';
    return pathNameSize;
}

static unsigned int getNum(char *str, int *err) {
   unsigned int val;
   char *endptr;

   *err = 0;
   val = strtoll(str, &endptr, 0);
   if (*endptr) {*err = 1; val = -1;}
   return val;
}

static void usage() {
   fprintf(stderr, "\n" \
      "Usage: sudo xxxx [OPTION] ...\n" \
      "   -p value, socket port, 1024-32000, default 9999\n" \
      "EXAMPLE\n" \
      "sudo xxxx -p 9998\n" \
   "\n");
}

static void initOpts(int argc, char *argv[]) {
   int opt, err, i;
   unsigned int mask;

   while ((opt = getopt(argc, argv, "dp:l:r:x:")) != -1) {
      switch (opt) {
         case 'd':
            g_bRunAsDaemon = false;
            break;
         case 'p':
            i = getNum(optarg, &err);
            socketPort = i;
            break;
         case 'l':
            logfile = strdup(optarg);
            break;
         case 'r':
            i= getNum(optarg, &err);
            g_RepeatDefault = i;
            break;
         case 'x':
            i= getNum(optarg, &err);
            g_DelayDefault = i;
            break;

        default: /* '?' */
           usage();
           exit(-1);
      }
   }
}

int pirfserver_start(int socketPort);
int pirfserver_stop();

int main(int argc, char **argv) {
    
	//ignore pipe errors
	signal(SIGPIPE, SIG_IGN);

    g_bRunAsDaemon = true;	

    /* check command line parameters */
    initOpts(argc, argv);
    
	if (g_bRunAsDaemon) {
		setlogmask(LOG_UPTO(LOG_INFO));
		openlog(DAEMON_NAME, LOG_CONS | LOG_PERROR, LOG_USER);
		syslog(LOG_INFO, "pirflinx is starting up");
	}
    
    char szStartupPath[255];
    getExecutablePathName((char*)&szStartupPath,255);
    szStartupFolder=szStartupPath;
    if (szStartupFolder.find_last_of('/')!=std::string::npos)
        szStartupFolder=szStartupFolder.substr(0,szStartupFolder.find_last_of('/')+1);
    
	if (g_bRunAsDaemon)
		/* Deamonize */
		daemonize(szStartupFolder.c_str(), PID_FILE);
        
    // Need here to start main worker thread
    pirfserver_start(socketPort);
    
	while (!g_bStopApplication) {
		sleep(1);
	}
	fflush(stdout);
	_log.Log(LOG_STATUS, "Stopping worker...");
    
    // Stop main worker ???
    pirfserver_stop();

	if (g_bRunAsDaemon) {
		syslog(LOG_INFO, "pirflinx stopped");
		daemonShutdown();
		// Delete PID file
		remove(PID_FILE);
	}    
	return 0;
}

