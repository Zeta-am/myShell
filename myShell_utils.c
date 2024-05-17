#include "myShell_utils.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/param.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/wait.h>

/*** Function declaration ***/
void parse_command(char *, char **, char*);
void get_relative_cwd(char *);
int exec_builtin(char **);
int is_badly_formatted(char *);
int count_occurrences(char*, char);

/*
 * Count the occurrences of target in str
 * */
int count_occurrences(char *str, char target) {
    int out = 0;

    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == target) {
            out++;
        }
    }
    return out;
}


/*
 * Execute a builtin command
 * */
int exec_builtin(char **args) {

    // Parse the command
    if (strcmp(args[0], "quit") == 0) {
        exit(0);
    } else if (strcmp(args[0], "cd") == 0) {
        // If only 'cd' has been inserted, change directory to the home directory
        if (args[1] == NULL) {
            if (chdir(getenv("HOME")) == -1) {
                perror("chdir");
                return EXIT_FAILURE;
            }
            return 1;
        }
        if (chdir(args[1]) == -1) {
            perror("chdir");
            return EXIT_FAILURE;
        }
        return 2;
    }
    // No command builtin has been executed
    return 0;
}


/*
 * Print the current working directory
 * */
void get_relative_cwd(char *cwd) {

    // Get the current working directory
    if (getcwd(cwd, MAXPATHLEN) == NULL) {
        handle_error("getcwd");
    }

    // Save the HOME variable
    char *home = NULL;
   
    if ((home = getenv("HOME")) == NULL) {
        handle_error("getenv");
    }

    // Check if in the cwd occurs the home path
    if (strstr(cwd, home) == cwd) {
        memmove(cwd, "~", 1);   // Replace the base path with ~
        memmove(cwd + 1, cwd + strlen(home), strlen(cwd) - strlen(home) + 1);    // Concatenate the rest of the string
    }

}


/*
 * Print the prompt
 * */
void print_prompt() {
    char cwd[MAXPATHLEN], hostname[256];           // To save the current working directory and the hostname
    char *username = getlogin();    // To save the username

    // Get the relative cwd
    get_relative_cwd(cwd);
    
    // Get the hostname
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        handle_error("gethostname");
    } 
    // Truncate the hostname, replacing the '.' with '\0'
    // strchr return a pointer to '.' into hostname
    char *dot = strchr(hostname, '.');
    if (dot != NULL) {
        *dot = '\0';
    }

    // Save the prompt into prompt
    printf("\033[1;32m%s\033[0m at \033[1;34m%s\033[0m in \033[1;33m%s\033[0m > ", username, hostname, cwd);
}

/*
 * Parse in option a command
 * */
void parse_command(char *command, char **args, char *sep) {
    int i = 0;
    args[i++] = strtok(command, sep);

    // Spilt the command into an array of token
    for ( ; i < MAX_ARGUMENTS && (args[i] = strtok(NULL, sep)) != NULL; i++) {}
}


/*
 *  Execute the command comm
 * */
void execute_command(char *comm) {
    // Check if the line is longer than the maximum length
    if (strlen(comm) > MAX_COMMAND_LENGTH) {
        fprintf(stderr, "The line is too long\n");
        return;
    }

    //Divide the command into arguments
    char *arguments[MAX_ARGUMENTS];
    parse_command(comm, arguments, " \t\n");

    // If is a builtin command, don't create a process
    if (exec_builtin(arguments)) {
        return;
    }

    // Create a process child
    pid_t pid = fork();

    // Check if there was an error
    if (pid == -1) {
        handle_error("fork");
    }
    
    if (pid == 0) {
        // Child process

        // Execute the command
        execvp(arguments[0], arguments);

        // If the program reaches this line, there was an error with the execvp
        handle_error("execvp");
    } else {
        // Parent process

        // Wait for the termination of the child process
        if (wait(NULL) == -1) {
            handle_error("wait");
        }
    }

}

/*
 *  Split the commands comms into single command
 *  And create a thread for each
 * */
void execute_commands(char *comms) { 
    // Check if the line is longer than the maximum lengt or if is badly formattedh
    if (strlen(comms) > MAX_COMMAND_LENGTH || is_badly_formatted(comms)) {
        fprintf(stderr, "The line is too long\n");
        return;
    }
    
    // Count the number of occurrences of ';'
    int n = count_occurrences(comms, ';');
    
    // Parse the line
    char *args[MAX_COMMAND_LENGTH];
    parse_command(comms, args, ";");
    
    // Save the index of the quit command, if it has been inserted
    int quitIndex = -1;
    for (int i = 0; i < n + 1; i++) {
        if (strstr(args[i], "quit") != NULL) {
            quitIndex = i;
        }
    }
    // The number of thread to execute
    // If the user has inserted quit, execute the commands up to quitIndex-1
    int threads_length = quitIndex == -1 ? n + 1 : quitIndex;

    // Declares  an array of threads 
    pthread_t threads[threads_length];

    // Create a thread for each command
    for (int i = 0; i < threads_length; i++) {
        if (pthread_create(threads + i, NULL, (void *) &execute_command, args[i]) != 0) {
            handle_error("pthread_create");
        } 
    }

    // Wait for the termination of the threads
    for (int i = 0; i < threads_length; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            handle_error("pthread_join");
        }
    }

    // If quit has been inserted, quit the shell after the execution of the commands
    char *quit[] = {"quit"};
    if (quitIndex != -1) {
        exec_builtin(quit);
    }
}


/*
 * Check if the line is badly formatted
 * */
int is_badly_formatted(char *line) {
    if (line[0] == ';' || line[strlen(line)-1] == ';') {
        return 1;
    }

    // Cycle through the string
    for (int i = 0; i < strlen(line)-2 && line[i] != '\0'; i++) {
        // If there are two consecutives ';', error
        if (line[i] == ';' && line[i+1] == ';') {
            return 1;
        } 
        // If a blank char divide two ';', error
        if (line[i] == ';' && line[i+2] == ';' && isblank(line[i+1])) {
            return 2;
        }
    }
    // The line is goodly formatted
    return 0;
}

/*
 * Print the banner
 * */
void print_banner() {
    puts("                          ______   __                  __  __ ");
    puts("                         /      \\ /  |                /  |/  |");
    puts(" _____  ____   __    __ /$$$$$$  |$$ |____    ______  $$ |$$ |");
    puts("/     \\/    \\ /  |  /  |$$ \\__$$/ $$      \\  /      \\ $$ |$$ |");
    puts("$$$$$$ $$$$  |$$ |  $$ |$$      \\ $$$$$$$  |/$$$$$$  |$$ |$$ |");
    puts("$$ | $$ | $$ |$$ |  $$ | $$$$$$  |$$ |  $$ |$$    $$ |$$ |$$ |");
    puts("$$ | $$ | $$ |$$ \\__$$ |/  \\__$$ |$$ |  $$ |$$$$$$$$/ $$ |$$ |");
    puts("$$ | $$ | $$ |$$    $$ |$$    $$/ $$ |  $$ |$$       |$$ |$$ |");
    puts("$$/  $$/  $$/  $$$$$$$ | $$$$$$/  $$/   $$/  $$$$$$$/ $$/ $$/");
    puts("              /  \\__$$ |                                      ");
    puts("              $$    $$/                                       ");
    puts("               $$$$$$/                                        ");
    puts("\nWelcome to myShell\n");
}
