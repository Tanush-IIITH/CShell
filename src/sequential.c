#include "sequential.h"
#include "command.h"
#include "header.h"

/**
 * Execute sequential commands separated by semicolons
 * 
 * This function implements the semicolon operator (;) for sequential command execution.
 * Commands are executed one after another, with each command waiting for the previous
 * to complete before starting. Unlike pipes, commands don't share data - they're
 * completely independent executions.
 * 
 * Syntax: command1 ; command2 ; ... ; commandN
 * 
 * Behavior:
 * - Each command is executed in order
 * - Shell waits for each command to complete before starting the next
 * - If a command fails, subsequent commands still execute
 * - Each command can contain pipes, redirections, etc. (full shell_cmd)
 * - Shell prompt only appears after ALL commands complete
 * 
 * Examples:
 * - "echo hello ; echo world ; echo done"
 * - "ls -l ; cat file.txt ; echo finished"
 * - "echo data > file.txt ; cat file.txt ; rm file.txt"
 * - "cat file1.txt | grep pattern ; echo done ; ls"
 * 
 * @param command: Full command string containing semicolon operators
 */
void execute_sequential_commands(char *command) {
    // Skip processing if command is empty or NULL
    if (command == NULL || strlen(command) == 0) {
        return;
    }
    
    // Step 1: Create a working copy since we'll modify the string during parsing
    char *command_copy = strdup(command);
    if (command_copy == NULL) {
        perror("strdup");
        return;
    }
    
    // Step 2: Count the total number of commands in the sequence
    // Use helper function for consistent counting logic
    int num_commands = count_sequential_commands(command);
    
    // Step 3: Execute each command sequentially
    // Note: Since we only reach this function when semicolons are detected,
    // num_commands will always be >= 2
    char *cmd_start = command_copy;  // Pointer to start of current command
    
    for (int i = 0; i < num_commands; i++) {
        // Step 3a: Extract current command from the sequence string
        // Find next semicolon operator or end of string
        char *semicolon_pos = strchr(cmd_start, ';');
        if (semicolon_pos) {
            *semicolon_pos = '\0';  // Null-terminate current command string
        }
        
        // Step 3b: Trim whitespace from the beginning and end of command
        // Skip leading whitespace
        while (*cmd_start == ' ' || *cmd_start == '\t') {
            cmd_start++;
        }
        
        // Find end of command and trim trailing whitespace
        char *cmd_end = cmd_start + strlen(cmd_start) - 1;
        while (cmd_end > cmd_start && (*cmd_end == ' ' || *cmd_end == '\t')) {
            *cmd_end = '\0';
            cmd_end--;
        }
        
        // Step 3c: Skip empty commands (e.g., "cmd1 ; ; cmd2" - ignore empty middle)
        if (strlen(cmd_start) == 0) {
            if (semicolon_pos) {
                cmd_start = semicolon_pos + 1;
            }
            continue;
        }
        
        // Step 3d: Execute the current command
        // Each command is treated as a complete shell command that can contain:
        // - Single commands: "echo hello"
        // - Pipelines: "cat file.txt | grep pattern | wc -l"
        // - Redirections: "echo data > file.txt"
        // - Complex combinations: "cat < input.txt | grep pattern > output.txt"
        
        execute_command_without_logging(cmd_start);
        
        // Step 3e: Command has completed, move to next command in the sequence
        if (semicolon_pos) {
            cmd_start = semicolon_pos + 1;
            // Note: We'll trim whitespace at the start of the next iteration
        }
        
        // No need to wait explicitly - execute_command_without_logging() already waits
        // for command completion before returning
    }
    
    // Step 4: Cleanup
    free(command_copy);
    
    // All sequential commands have completed
    // Shell prompt will be displayed by the calling function
}

/**
 * Check if a command string contains semicolon operators
 * 
 * This is a utility function to determine if a command needs sequential
 * execution handling or can be processed as a single command/pipeline.
 * 
 * @param command: Command string to check
 * @return: 1 if semicolons are present, 0 otherwise
 */
int contains_semicolon(const char *command) {
    if (command == NULL) {
        return 0;
    }
    
    return (strchr(command, ';') != NULL) ? 1 : 0;
}

/**
 * Count the number of commands in a semicolon-separated sequence
 * 
 * Utility function to count how many individual commands are present
 * in a semicolon-separated command string.
 * 
 * @param command: Command string to analyze
 * @return: Number of individual commands
 */
int count_sequential_commands(const char *command) {
    if (command == NULL || strlen(command) == 0) {
        return 0;
    }
    
    int count = 1;  // At least one command if string is not empty
    const char *temp = command;
    
    while ((temp = strchr(temp, ';')) != NULL) {
        count++;
        temp++; // Move past the current semicolon
    }
    
    return count;
}
