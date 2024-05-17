#include <stdio.h>
#include "myShell_utils.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>



int main(int argc, char **argv) {

    // If the number of argument is greater than 2 exit from the program
    if (argc > 2) {
        fprintf(stderr, "usage: ./myShell [batchFile]\n");
        return EXIT_FAILURE;
    }

    FILE *fp = NULL;                       // To save the descriptor of the batch file if specified
    
    char line[MAX_COMMAND_LENGTH];    // To save the command insert
    
    
    // Check if the shell is started in batch or interactive mode
    if (argc > 1) {
        // Open the file and save the descriptor
        if ((fp = fopen(argv[1], "r" )) == NULL) {
            handle_error("fopen");
        }
    }

    // Print the welcome banner
    print_banner();
    while(1) {
        if (fp == NULL) {
            /* Interactive mode */
            
            // Read from the prompt
            print_prompt();
            if (fgets(line, MAX_COMMAND_LENGTH, stdin) == NULL) {
                break;
            }
                                                
        } else {
            /* Batch mode */
            // Read the command/commands from the batch file
            if (fgets(line, MAX_COMMAND_LENGTH, fp) == NULL) {
                break;
            }
            // Print the line that is about to be executed
            printf("\n\033[1;32mExecuting\033[0m: \e[1m%s\e[m\n\n", line);    
        }

        // Remove the newline if the line is different by '\n'
        if (strcmp(line, "\n") == 0) {
             continue; 
        } else {
            line[strlen(line)-1] = '\0';
        }
        // Execute the commands 
        if (strchr(line, ';') != NULL) {
            execute_commands(line);
        } else { 
            execute_command(line);
        }
    }
    // Close the file if it was open
    if (fp != NULL) {
        fclose(fp);
    }

    return EXIT_SUCCESS;
}
