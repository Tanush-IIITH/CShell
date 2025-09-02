#include "../include/header.h"
#include "../include/command.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/log.h"

/**
 * Parse command string into arguments array with input redirection support
 * 
 * This function takes a command string (e.g., "hop /tmp .." or "cat < file.txt") 
 * and breaks it down into individual arguments that can be processed by command handlers.
 * It handles input redirection with the < operator.
 * 
 * @param command: Input command string to be parsed (will be modified by strtok)
 * @param arg_count: Pointer to integer that will store the number of arguments found
 * @param input_file: Pointer to string pointer that will store input redirection file (or NULL)
 * @return: Array of string pointers containing the parsed arguments, NULL-terminated
 * 
 * Memory allocation: Caller is responsible for freeing the returned array and its contents
 * using free_command_args(), and freeing input_file if not NULL
 */
char** parse_command_args(char *command, int *arg_count, char **input_file) {
    // Allocate memory for argument pointers array (support up to 64 arguments)
    // Each element will point to a dynamically allocated string
    char **args = malloc(64 * sizeof(char*));
    *arg_count = 0;  // Initialize argument counter
    *input_file = NULL;  // Initialize input file pointer
    
    // Clean up the command string by removing trailing newline character
    // This is necessary because input from fgets() often includes '\n'
    char *newline = strchr(command, '\n');
    if (newline) {
        *newline = '\0';  // Replace newline with null terminator
    }
    
    // Begin tokenization process using strtok()
    // Split on space, tab, newline, and carriage return characters
    char *token = strtok(command, " \t\n\r");
    
    // Process each token until we reach the end or hit our argument limit
    while (token != NULL && *arg_count < 63) { // Reserve space for NULL terminator
        if (strcmp(token, "<") == 0) {
            // Input redirection operator found
            token = strtok(NULL, " \t\n\r");  // Get the filename
            if (token != NULL) {
                // If multiple input redirections, only keep the last one
                if (*input_file) {
                    free(*input_file);
                }
                *input_file = strdup(token);
            }
        } else {
            // Regular argument - create a permanent copy of the token using strdup()
            // This is necessary because strtok() returns pointers to the original string
            args[*arg_count] = strdup(token);
            (*arg_count)++;  // Increment argument counter
        }
        
        // Get the next token from the remaining string
        token = strtok(NULL, " \t\n\r");
    }
    
    // NULL-terminate the arguments array (required by execv and similar functions)
    args[*arg_count] = NULL;
    return args;
}

/**
 * Free memory allocated for command arguments
 * 
 * This function properly deallocates all memory associated with a command arguments array.
 * It first frees each individual argument string, then frees the array itself.
 * 
 * @param args: Array of string pointers to be freed
 * @param arg_count: Number of arguments in the array
 * 
 * Note: This function safely handles NULL pointers and should always be called
 * after using parse_command_args() to prevent memory leaks.
 */
void free_command_args(char **args, int arg_count) {
    if (args) {  // Check if args pointer is valid
        // Free each individual argument string
        for (int i = 0; i < arg_count; i++) {
            free(args[i]);  // Free the string allocated by strdup()
        }
        // Free the array of pointers itself
        free(args);
    }
}

/**
 * Setup input redirection for a command
 * 
 * @param input_file: Path to the input file
 * @return: File descriptor of the opened file, or -1 on error
 */
int setup_input_redirection(const char *input_file) {
    int fd = open(input_file, O_RDONLY);
    if (fd == -1) {
        printf("No such file or directory\n");
        return -1;
    }
    
    // Redirect stdin to the file
    if (dup2(fd, STDIN_FILENO) == -1) {
        perror("dup2");
        close(fd);
        return -1;
    }
    
    return fd;  // Return fd so caller can close it
}

/**
 * Execute a command entered by the user
 * 
 * This is the main command dispatcher that:
 * 1. Parses the input command string into arguments
 * 2. Identifies the command type (built-in vs external)
 * 3. Calls the appropriate command handler
 * 4. Manages memory cleanup
 * 5. Adds successful commands to log
 * 
 * @param command: Raw command string entered by the user
 * 
 * Current supported built-in commands:
 * - hop: Directory navigation command
 * - reveal: Directory listing command
 * - log: Command history management
 * - exit: Terminate the shell
 */
void execute_command(char *command) {
    // Skip processing if command is empty or NULL
    // This handles cases where user just presses Enter
    if (command == NULL || strlen(command) == 0) {
        return;  // Nothing to execute
    }

    // Remove trailing newline for logging
    char *command_for_log = strdup(command);
    char *newline = strchr(command_for_log, '\n');
    if (newline) {
        *newline = '\0';
    }

    // Execute the command
    execute_command_without_logging(command);

    // Add to log after successful execution
    add_to_log(command_for_log);
    
    free(command_for_log);
}

/**
 * Execute a command without adding it to the log
 * Used by log execute functionality to avoid recursive logging
 */
void execute_command_without_logging(char *command) {
    // Skip processing if command is empty or NULL
    // This handles cases where user just presses Enter
    if (command == NULL || strlen(command) == 0) {
        return;  // Nothing to execute
    }
    
    // Create a working copy of the command string
    // This is necessary because parse_command_args() uses strtok() which modifies the string
    char *command_copy = strdup(command);
    
    // Parse the command into an arguments array with redirection support
    int arg_count;
    char *input_file;
    char **args = parse_command_args(command_copy, &arg_count, &input_file);
    
    // Handle edge case where parsing resulted in no arguments
    if (arg_count == 0) {
        // Clean up allocated memory before returning
        free(command_copy);
        free_command_args(args, arg_count);
        if (input_file) free(input_file);
        return; // Nothing to execute
    }
    
    // Special case: exit command must run in parent to actually exit the shell
    if (strcmp(args[0], "exit") == 0) {
        printf("Goodbye!\n");
        exit(0);  // Terminate the program immediately
    }
    
    // All other commands (built-in and external) run in child process for consistent I/O handling
    pid_t pid = fork();  // Create a child process
    
    if (pid == 0) {
        // Child process: Set up input redirection if needed, then execute command
        
        // Set up input redirection if specified
        if (input_file != NULL) {
            if (setup_input_redirection(input_file) == -1) {
                // Failed to open file or redirect - error already printed
                exit(1);
            }
        }
        
        // Command dispatch: Check the first argument to determine command type
        if (strcmp(args[0], "hop") == 0) {
            // Built-in hop command for directory navigation
            hop_command(args, arg_count);
            exit(0);  // Exit child process after built-in command
        } else if (strcmp(args[0], "reveal") == 0) {
            // Built-in reveal command for listing directory contents
            reveal_command(args, arg_count);
            exit(0);  // Exit child process after built-in command
        } else if (strcmp(args[0], "log") == 0) {
            // Built-in log command for command history
            log_command(args, arg_count);
            exit(0);  // Exit child process after built-in command
        } else {
            // External command: Use execvp() to execute arbitrary programs
            // execvp() searches for the command in PATH and executes it
            if (execvp(args[0], args) == -1) {
                // execvp() failed - silently ignore and exit
                exit(1);  // Exit child process with error status (but no message)
            }
        }
    } else if (pid > 0) {
        // Parent process: Wait for child to complete
        int status;
        waitpid(pid, &status, 0);  // Wait for the child process to finish
    } else {
        // fork() failed
        perror("fork");  // Print system error message
    }
    
    // Memory cleanup: Always free allocated resources
    free(command_copy);              // Free the duplicated command string
    free_command_args(args, arg_count);  // Free the arguments array and its contents
    if (input_file) free(input_file);    // Free input file string if allocated
}
