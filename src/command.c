#include "../include/header.h"
#include "../include/command.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/log.h"

/**
 * Parse command string into arguments array
 * 
 * This function takes a command string (e.g., "hop /tmp ..") and breaks it down
 * into individual arguments that can be processed by command handlers.
 * 
 * @param command: Input command string to be parsed (will be modified by strtok)
 * @param arg_count: Pointer to integer that will store the number of arguments found
 * @return: Array of string pointers containing the parsed arguments, NULL-terminated
 * 
 * Memory allocation: Caller is responsible for freeing the returned array and its contents
 * using free_command_args()
 */
char** parse_command_args(char *command, int *arg_count) {
    // Allocate memory for argument pointers array (support up to 64 arguments)
    // Each element will point to a dynamically allocated string
    char **args = malloc(64 * sizeof(char*));
    *arg_count = 0;  // Initialize argument counter
    
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
        // Create a permanent copy of the token using strdup()
        // This is necessary because strtok() returns pointers to the original string
        args[*arg_count] = strdup(token);
        (*arg_count)++;  // Increment argument counter
        
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
    
    // Parse the command into an arguments array
    int arg_count;
    char **args = parse_command_args(command_copy, &arg_count);
    
    // Handle edge case where parsing resulted in no arguments
    if (arg_count == 0) {
        // Clean up allocated memory before returning
        free(command_copy);
        free_command_args(args, arg_count);
        return; // Nothing to execute
    }
    
    // Command dispatch: Check the first argument to determine command type
    if (strcmp(args[0], "hop") == 0) {
        // Built-in hop command for directory navigation
        hop_command(args, arg_count);
    } else if (strcmp(args[0], "reveal") == 0) {
        // Built-in reveal command for listing directory contents
        reveal_command(args, arg_count);
    } else if (strcmp(args[0], "log") == 0) {
        // Built-in log command for command history
        log_command(args, arg_count);
    } else if (strcmp(args[0], "exit") == 0) {
        // Built-in exit command to terminate shell
        printf("Goodbye!\n");
        exit(0);  // Terminate the program immediately
    } else {
        // External command: Use fork() and execvp() to execute arbitrary programs
        pid_t pid = fork();  // Create a child process
        
        if (pid == 0) {
            // Child process: Execute the external command
            // execvp() searches for the command in PATH and executes it
            if (execvp(args[0], args) == -1) {
                // execvp() failed - command not found or execution error
                printf("Invalid Syntax!\n");
                exit(1);  // Exit child process with error status
            }
        } else if (pid > 0) {
            // Parent process: Wait for child to complete
            int status;
            waitpid(pid, &status, 0);  // Wait for the child process to finish
        } else {
            // fork() failed
            perror("fork");  // Print system error message
        }
    }
    
    // Memory cleanup: Always free allocated resources
    free(command_copy);              // Free the duplicated command string
    free_command_args(args, arg_count);  // Free the arguments array and its contents
}
