#ifndef _SHELL_H_
#define _SHELL_H_

#include <termios.h>
#include "sys/types.h"

// file descripter for the terminal
int shell_terminal;
// 1 if shell_terminal is a valid terminal, 0 otherwise
int shell_is_interactive;
// terminal options for the shell
struct termios shell_tmodes;
// the shell's process id
pid_t shell_pgid;
int shell(int argc, char *argv[]);

char* get_pwd(void);

#endif
