#define _GNU_SOURCE
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define INPUT_STRING_SIZE 80

#include "io.h"
#include "parse.h"
#include "process.h"
#include "shell.h"

char* get_pwd(void){
	return(get_current_dir_name());
}

/* Built-in command: pwd */
int cmd_pwd() {
	fprintf(stdout, "%s\n", get_pwd());
	return 1;
}

/* Built-in command: cd */
char* prev_dir;
int cmd_cd(tok_t arg[]) {

	char* change_dir=arg[0];
	char first[256], firsttwo[256];
	strncpy(first, &change_dir[0], 1);
	strncpy(firsttwo, &change_dir[0], 2);
	int err=0;

	// CASE - directory starts with ..
	if ( strcmp(firsttwo, "..") == 0 ) {
		prev_dir=get_pwd();
		char tmp[256];

		bool b=true;
		int i=strlen(prev_dir)-1, index=0;
		do {
			if ( prev_dir[i] == '/' ) {
				index=i;
				b=false;
			}
			i--;
		} while(b);
		strncpy(tmp, prev_dir, index);
		if ( strlen(change_dir)>2 )
  			strncat(tmp, change_dir+2, strlen(change_dir)-2);
  		err=chdir(tmp);
  		if (err==-1)
			printf("shell: cd: %s: No such file or directory\n", tmp);
	}
	// CASE - directory starts with .
	else if ( strcmp(first, ".") == 0 ) {
		prev_dir=get_pwd();
		char* tmp=malloc(sizeof(char*));
		tmp=get_pwd();
	  	strncat(tmp, &change_dir[1], strlen(change_dir)-1);
	  	err=chdir(tmp);
	  	if (err==-1)
			printf("shell: cd: %s: No such file or directory\n", tmp);
	}
	// CASE - directory starts with -
	else if ( strcmp(first, "-") == 0 ) {
		char *tmp_dir=get_pwd();
		chdir(prev_dir);
		prev_dir=tmp_dir;
	}
	// CASE - directory starts with ~
	else if ( strcmp(first, "~") == 0 ) {
		char home_dir[80];
		char* user=getenv("USER");
		strcpy(home_dir, "/home/");
		strcat(home_dir, user);

		int n=strlen(&change_dir[0]);
		if (n>1)
			strcat(home_dir, &change_dir[1]);
		prev_dir=get_pwd();
		err=chdir(home_dir);
		if (err==-1)
			printf("shell: cd: %s: No such file or directory\n", home_dir);
	}
	//CASE - directory contains no wildcards
	else if ( strcmp(first, "/") == 0 ) {
		prev_dir=get_pwd();
		err=chdir(change_dir);
		if (err==-1)
			printf("shell: cd: %s: No such file or directory\n", change_dir);
	}
	//CASE - deal with the rest
	else {
		prev_dir=get_pwd();
		char* tmp=malloc(3*sizeof(char*));
		strcpy(tmp, get_pwd());
		strcat(tmp, "/");
		strcat(tmp, change_dir);
		err=chdir(tmp);
		if (err==-1)
			printf("shell: cd: %s: No such file or directory\n", tmp);
	}

	return 1;
}

/* Built-in command: quit */
int cmd_quit() {
  printf("Bye\n");
  exit(0);
  return 1;
}

/* Built-in command: help */
int cmd_help();

void wc (FILE *infile, char *inname, int *totWC, int *totLC, int *totBC) {
	int wordCount = 0, lineCount = 0;
	char line[256];

	if (infile == NULL) {
		printf("wc: %s: No such file or directory\n", inname);
		return;
	}

	//Getting lineCount and wordCount
	while (fgets(line, sizeof(line), infile)) {
		lineCount++;
		for (unsigned int i=0; i<strlen(line); i++) {
			if ( (line[i] != ' ') && ( (line[i+1] == ' ') || (line[i+1] == '\n') )  )
				wordCount++;
		}
	}

	//Getting byteCount
	fseek(infile, 0, SEEK_END);
	int byteCount = ftell(infile);

	//Adding to totals
	*totLC+=lineCount;
	*totWC+=wordCount;
	*totBC+=byteCount;

	printf("%3d %3d %3d %3s\n", lineCount, wordCount, byteCount, inname);
	fclose(infile);
}

int cmd_wc (tok_t arg[]) {
	int *totWC = malloc(sizeof(int)), *totLC = malloc(sizeof(int)), *totBC = malloc(sizeof(int));
	*totWC = 0; *totLC = 0; *totBC = 0;
	/*int n = sizeof(arg) / sizeof(arg[0]);
	printf("n is %d", n);*/
	for (int i=0; arg[i]!=NULL; i++) {
		FILE *infile = fopen(arg[i], "r");
		wc(infile, arg[i], totWC, totLC, totBC);
	}
	printf("%3d %3d %3d total\n", *totLC, *totWC, *totBC);
	return 1;
}

/* Command Lookup Table Structures */
typedef int cmd_fun_t (tok_t args[]);
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_quit, "quit", "quit the command shell"},
  {cmd_pwd, "pwd", "print current working directory"},
  {cmd_cd, "cd", "change current working directory"},
  {cmd_wc, "wc", "word, line, and letter count for files"}
};

int cmd_help() {
  unsigned int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    printf("%s - %s\n",cmd_table[i].cmd, cmd_table[i].doc);
  }
  return 1;
}

int lookup(char cmd[]) {
  unsigned int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)) return i;
  }
  return -1;
}

void init_shell() {
  // check if we are running interactively
  shell_terminal = STDIN_FILENO;

  // note that we cannot take control of the terminal if the shell is not interactive
  shell_is_interactive = isatty(shell_terminal);

  if(shell_is_interactive) {

    // force into foreground
    while(tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp()))
      kill( - shell_pgid, SIGTTIN);

    shell_pgid = getpid();
    // put shell in its own process group
    if(setpgid(shell_pgid, shell_pgid) < 0) {
      perror("Couldn't put the shell in its own process group");
      exit(1);
    }

    // Take control of the terminal
    tcsetpgrp(shell_terminal, shell_pgid);
    tcgetattr(shell_terminal, &shell_tmodes);
  }

  // ignore signals
  signal(SIGINT, SIG_IGN);
  signal(SIGKILL, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGCONT, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
}

/**
 * Add a process to our process list
 */
process* tmp;
void add_process(process* p) {
  if (first_process==NULL) {
  	first_process=p;
  	tmp=first_process;
  	return;
  }
  tmp->next=p;
  p->prev=tmp;
  tmp=tmp->next;
}

/**
 * Creates a process given the tokens parsed from stdin
 *
 */
process* create_process(tok_t* tokens) {

  process* p=malloc(sizeof(process*));

  int out=isDirectTok(tokens, ">");
  int in=isDirectTok(tokens, "<");

  if ( (out) || (in) ) {

  	p->argv=malloc(MAXTOKS);
  	int limit=0;

		// output redirection
		if (out) {
			p->stdout=open(tokens[out+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			limit=out;
		}

		// input redirection
		if (in) {
			p->stdin=open(tokens[in+1], O_RDONLY);
			limit=in;
		}

		int i;
		for (i=0; i<limit; i++)
			p->argv[i]=tokens[i];

	}
	// standard input and output
	else
 		p->argv=tokens;

	p->argc=arrayCount(p->argv);
	p->prev=NULL;
    p->next=NULL;

    return p;

}

void shell_prompt(int *lineNum, char *curr_dir) {
	*lineNum = *lineNum+1;
    curr_dir=get_pwd();
    fprintf(stdout, "%d %s: ", *lineNum, curr_dir);
}

int shell (__attribute__ ((unused)) int argc, char *argv[]) {
  prev_dir=get_pwd();

  // get current process's PID
  pid_t pid = getpid();
  // get parent's PID
  pid_t ppid = getppid();
  // use this to store a child PID
  // pid_t cpid;

  // user input string
  char *s = malloc(INPUT_STRING_SIZE+1);
  // tokens parsed from input
  tok_t *t;

  int *lineNum = malloc(sizeof(int));
  *lineNum=-1;
  char* curr_dir=get_pwd();
  int fundex = -1;

  // perform some initialisation
  init_shell();
  printf("%s running as PID %d under %d\n",argv[0],pid,ppid);
  shell_prompt(lineNum, curr_dir);

  while ((s = freadln(stdin))) {
	// break the line into tokens
    t = getToks(s);
	// checking if first token is a shell literal
    fundex = lookup(t[0]);
    if (fundex >= 0)
		cmd_table[fundex].fun(&t[1]);
	else if (fundex == -1)
		fprintf(stdout, "%s is not a built-in command\n", t[0]);
    else {
    	process* p=create_process(t);
    	launch_process(p);
    	//add_process(p);
    	//cpid=p->pid;
    }
	shell_prompt(lineNum, curr_dir);
  }
  return 0;
}
