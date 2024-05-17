#ifndef MYSHELL_UTIL_H
#define MYSHELL_UTIL_H

#include <stdlib.h>
/***** Macros  *****/

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while(0)

#define MAX_COMMAND_LENGTH 512

#define MAX_ARGUMENTS 32

/***** Functions Prototype  *****/
void execute_command(char *);
void execute_commands(char *);
void print_banner();
void print_prompt();

#endif
