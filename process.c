#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <termios.h>

#include "process.h"
#include "shell.h"
#include "parse.h"

/* Handles the execution of processes, signals, foreground and background processing */

extern char *strdup(const char *s);
extern pid_t getpgrp(void);
extern int kill(pid_t pid, int sig);

void signalhandler(int sig) {
	switch(sig) {
		case SIGINT:
			printf("CTRL-C\n");
			kill(getpgrp(), SIGINT); break;
		case SIGKILL:
			printf("KILLING\n");
			kill(getpgrp(), SIGKILL); break;
		case SIGTERM:
			printf("TERMINATING\n");
			kill(getpgrp(), SIGTERM); break;
		case SIGQUIT:
			printf("CTRL-\\\n");
			kill(getpgrp(), SIGQUIT); break;
		case SIGTSTP:
			printf("CTRL-Z\n");
			kill(getpgrp(), SIGTSTP); break;
		case SIGCONT:
			printf("RESUMING\n");
			kill(getpgrp(), SIGCONT); break;
		case SIGTTIN:
			printf("PAUSING\n");
			kill(getpgrp(), SIGTTIN); break;
		case SIGTTOU:
			printf("PAUSING\n");
			kill(getpgrp(), SIGTTOU); break;
	}
}

/**
 * Executes the process p.
 * If the shell is in interactive mode and the process is a foreground process,
 * then p should take control of the terminal.
 */
void launch_process(process *p) {
  char* program=p->argv[0];

  if ( program[0] == '.' ) {
  	char* tmp=get_pwd();
  	strncat(tmp, program+1, strlen(program)-1);
  	program=tmp;
  }
  else if ( program[0] != '/' ) {

  	// splits path environement variable and puts every path as an element in an array
  	char* dir[256];
  	int i=0;
		char* path_full=getenv("PATH");
		char* copy=(char*)malloc(strlen(path_full)+1);
		strcpy(copy, path_full);
		char* path_split=strtok(copy, ":");
		while( (path_split=strtok(NULL, ":")) ) {
			dir[i]=path_split;
			i++;
		}

		// concatenate / and the command to each path
		char* path_add;
		for (int j=0; j<i; j++) {
			path_add=strdup(dir[j]);
			strcat(path_add, "/");
			strcat(path_add, p->argv[0]);
			dir[j]=path_add;
		}

		// checks to see if path exists and picks the valid path
		int k=0, b=0;
		do {
			FILE *infile = fopen(dir[k], "r");
			if ( infile != NULL ) {
				b=1;
				program=dir[k];
				fclose(infile);
			}
			else
				k++;
		} while ( (b==0) && (k<i) );

		free(copy);
  }

  // executes process - also handles case if process does not exist
  p->pid=fork();
  setpgid(p->pid, p->pid+1);
  put_process_in_foreground (p, p->status);
  if (p->pid==0) {
  	if (p->stdout!=-1) {
  		dup2(p->stdout, 1);
  		close(p->stdout);
  	}
  	if (p->stdin!=-1) {
  		dup2(p->stdin, 0);
  		close(p->stdin);
  	}
	signal(SIGINT, signalhandler);
	signal(SIGKILL, signalhandler);
	signal(SIGTERM, signalhandler);
	signal(SIGQUIT, signalhandler);
	signal(SIGTSTP, signalhandler);
	signal(SIGCONT, signalhandler);
	signal(SIGTTIN, signalhandler);
	signal(SIGTTOU, signalhandler);
  	execv(program, p->argv);
  	printf("shell: %s: No such file or directory\n", program);
  	exit(0);
  }
  else {
  	wait(&p->status);
  	p->stdout=-1;
		p->stdin=-1;
  }
}

/**
 * Put a process in the foreground. This function assumes that the shell
 * is in interactive mode. If the cont argument is true, send the process
 * group a SIGCONT signal to wake it up.
 */
void
put_process_in_foreground (__attribute__ ((unused)) process *p, int cont) {
  /** CODE HERE */
  tcsetpgrp(shell_terminal, getpgrp());
  if (cont)
  	signal(SIGCONT, SIG_DFL);
}

/**
 * Put a process in the background. If the cont argument is true, send
 * the process group a SIGCONT signal to wake it up.
 */
void
put_process_in_background (__attribute__ ((unused)) process *p, __attribute__ ((unused)) int cont) {
  /** CODE HERE */
}
