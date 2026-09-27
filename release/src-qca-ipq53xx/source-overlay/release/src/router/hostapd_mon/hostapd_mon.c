#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <shared.h>

#define HAPD_MON_DBG(fmt, ...) printf("[hostapd_mon] " fmt "\n", ##__VA_ARGS__)
#define HOSTAPD_PATH "/usr/bin/hostapd"
#define MAX_ARGS 10

// copy from rc/common.c
static void killall_tk(const char *name)
{
	int n;

	if (killall(name, SIGTERM) == 0) {
		n = 10;
		while ((killall(name, 0) == 0) && (n-- > 0)) {
			_dprintf("%s: waiting name=%s n=%d\n", __FUNCTION__, name, n);
			usleep(100 * 1000);
		}
		if (n < 0) {
			n = 10;
			while ((killall(name, SIGKILL) == 0) && (n-- > 0)) {
				_dprintf("%s: SIGKILL name=%s n=%d\n", __FUNCTION__, name, n);
				usleep(100 * 1000);
			}
		}
	}
}

static void monitor_hostapd(pid_t hostapd_pid) {
	int status;
	char *args[] = {"rc", "rc_service", "restart_wireless", NULL};
	pid_t pid;

	HAPD_MON_DBG("[Parent] Monitor child process PID: %d", hostapd_pid);
	while (1) {
		pid_t result = waitpid(hostapd_pid, &status, 0);

		if (result < 0) {
			HAPD_MON_DBG("waitpid failed");
			exit(EXIT_FAILURE);
		} else if (WIFEXITED(status)) {
			HAPD_MON_DBG("hostapd exited with status %d", WEXITSTATUS(status));
			logmessage("HOSTAPD_MON", "hostapd exited with status %d", WEXITSTATUS(status));
			break;
		} else if (WIFSIGNALED(status)) {
			HAPD_MON_DBG("hostapd killed by signal %d", WTERMSIG(status));
			logmessage("HOSTAPD_MON", "hostapd killed by signal %d", WTERMSIG(status));
			break;
		} else {
			HAPD_MON_DBG("hostapd exited with undefine case");
			logmessage("HOSTAPD_MON", "hostapd exited with undefine case");
			break;
		}
	}

	HAPD_MON_DBG("[Parent] Hostapd crash, restart wireless");
	logmessage("HOSTAPD_MON", "Hostapd crash, restart wireless");

	_eval(args, "/dev/console", 0, &pid);
}

static void start_hostapd(char *hostapd_args[]) {
	HAPD_MON_DBG("[Child] Try to start hostapd, PID: %d ", getpid());
	logmessage("HOSTAPD_MON", "Start hostapd with PID:%d", getpid());

	execvp(HOSTAPD_PATH, hostapd_args);

	HAPD_MON_DBG("[Child] Failed to start hostapd");
	logmessage("HOSTAPD_MON", "Failed to start hostapd");
}

int main(int argc, char *argv[]) {
	pid_t hostapd_pid = 0;
	int i, index = 0;
	char *hostapd_args[MAX_ARGS];

	printf("[hostapd_mon] HOSTAPD ARGS: [ ");
	for (i = 1; i < argc; i++) {
		if (index >= MAX_ARGS - 1) {
			fprintf(stderr, "[hostapd_mon] Error: Too many arguments, maximum is %d\n", MAX_ARGS - 1);
			return EXIT_FAILURE;
		}
		printf("%s ", argv[i]);
		hostapd_args[index++] = argv[i];
	}
	printf("]\n");
	hostapd_args[index] = NULL;

	if (pids("hostapd")) {
		HAPD_MON_DBG("kill hostapd pid: %d", pids("hostapd"));
		killall_tk("hostapd");
		sleep(4);
	}

	hostapd_pid = fork();

	if (hostapd_pid < 0) {
		perror("Failed to fork");
		exit(EXIT_FAILURE);
	} else if (hostapd_pid == 0) { // Child
		start_hostapd(hostapd_args);
	} else { // Parent
		monitor_hostapd(hostapd_pid);
	}

	return 0;
}
