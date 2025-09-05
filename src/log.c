#include "../include/header.h"
#include "../include/log.h"
#include "../include/command.h"

/*
 * LOG SYSTEM OVERVIEW:
 * 
 * This module implements a persistent command history system with direct file access:
 * 1. Direct file operations (no in-memory buffer)
 * 2. Maximum 15 commands stored (oldest overwritten)
 * 3. File persistence across shell sessions
 * 4. Duplicate command prevention
 * 5. Command execution from history by index
 * 6. History purging capability
 * 
 * Commands are stored one per line in the file, with newest commands at the end.
 * When the limit is reached, the oldest commands are removed from the beginning.
 */

/**
 * Get the log file path (in shell project folder)
 * 
 * @return: Static string containing the full path to the log file
 */
static char* get_log_file_path() {
    static char log_path[PATH_MAX];
    snprintf(log_path, sizeof(log_path), "%s", LOG_FILE);
    return log_path;
}

/**
 * Read all commands from the log file into an array
 * 
 * @param commands: Array to store the commands
 * @param max_commands: Maximum number of commands to read
 * @return: Number of commands actually read
 */
static int read_all_commands(char commands[][4096], int max_commands) {
    FILE *file = fopen(get_log_file_path(), "r");
    if (!file) {
        return 0;  // File doesn't exist
    }
    
    int count = 0;
    char line[4096];
    while (fgets(line, sizeof(line), file) && count < max_commands) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        strncpy(commands[count], line, 4095);
        commands[count][4095] = '\0';
        count++;
    }
    
    fclose(file);
    return count;
}

/**
 * Write all commands from array to the log file
 * 
 * @param commands: Array of commands to write
 * @param count: Number of commands to write
 */
static void write_all_commands(char commands[][4096], int count) {
    FILE *file = fopen(get_log_file_path(), "w");
    if (!file) {
        perror("Failed to write log file");
        return;
    }
    
    for (int i = 0; i < count; i++) {
        fprintf(file, "%s\n", commands[i]);
    }
    
    fclose(file);
}

/**
 * Get the last command from the log file
 * 
 * @param last_command: Buffer to store the last command
 * @param buffer_size: Size of the buffer
 * @return: 1 if last command was found, 0 if file is empty or doesn't exist
 */
static int get_last_command(char *last_command, size_t buffer_size) {
    FILE *file = fopen(get_log_file_path(), "r");
    if (!file) {
        return 0;  // File doesn't exist
    }
    
    char line[4096];
    int found = 0;
    
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        strncpy(last_command, line, buffer_size - 1);
        last_command[buffer_size - 1] = '\0';
        found = 1;
    }
    
    fclose(file);
    return found;
}

/**
 * Check if command should be logged
 * 
 * @param command: The command string to evaluate
 * @return: 1 if command should be logged, 0 if it should be filtered out
 */
static int should_log_command(const char *command) {
    // Filter out empty or null commands
    if (!command || strlen(command) == 0) {
        return 0;
    }
    
    // Parse the command to extract the first word (command name)
    char *cmd_copy = strdup(command);
    char *token = strtok(cmd_copy, " \t\n\r");
    
    // Don't log "log" commands themselves
    if (token && strcmp(token, "log") == 0) {
        free(cmd_copy);
        return 0;
    }
    
    free(cmd_copy);
    
    // Check if it's identical to the last command
    char last_command[4096];
    if (get_last_command(last_command, sizeof(last_command))) {
        if (strcmp(last_command, command) == 0) {
            return 0;  // Duplicate of last command
        }
    }
    
    return 1;  // Command should be logged
}

/**
 * Add a command to the log
 * 
 * This function adds a command directly to the log file.
 * It handles the 15-command limit by removing old commands when necessary.
 * 
 * @param command: The command string to add to history
 * @return: 0 on success, non-zero on error
 */
int add_to_log(const char *command) {
    // Check if this command should be logged (filtering logic)
    if (!should_log_command(command)) {
        return 0;  // Command filtered out, but this is not an error
    }
    
    // Read all existing commands
    char commands[MAX_LOG_ENTRIES][4096];
    int count = read_all_commands(commands, MAX_LOG_ENTRIES);
    
    // If we're at the limit, shift all commands down (remove oldest)
    if (count >= MAX_LOG_ENTRIES) {
        for (int i = 0; i < MAX_LOG_ENTRIES - 1; i++) {
            strcpy(commands[i], commands[i + 1]);
        }
        count = MAX_LOG_ENTRIES - 1;
    }
    
    // Add the new command at the end
    strncpy(commands[count], command, 4095);
    commands[count][4095] = '\0';
    count++;
    
    // Write all commands back to file
    write_all_commands(commands, count);
    
    return 0;  // Success
}

/**
 * Clear the log (purge operation)
 * 
 * Removes all commands from the history by deleting the log file.
 */
void purge_log() {
    // Remove the log file completely
    const char *log_path = get_log_file_path();
    if (remove(log_path) != 0) {
        // File might not exist, which is fine for purge operation
        // No error message needed as the goal is achieved (no log file = empty log)
    }
}

/**
 * Print all log entries (oldest to newest)
 * 
 * Displays the command history in order from oldest to newest.
 * According to requirements: indexed newest to oldest for execute, but displayed oldest to newest.
 */
static void print_log() {
    char commands[MAX_LOG_ENTRIES][4096];
    int count = read_all_commands(commands, MAX_LOG_ENTRIES);
    
    if (count == 0) {
        return;  // No commands to display
    }
    
    // Print commands in order (oldest to newest)
    for (int i = 0; i < count; i++) {
        printf("%s\n", commands[i]);
    }
}

/**
 * Execute a command from log by index
 * 
 * Index is 1-based and indexed from newest to oldest according to requirements.
 * Index 1 = newest command, Index N = oldest command
 * 
 * @param index: 1-based index of command to execute (newest to oldest)
 * @return: 0 on success, -1 on error (invalid index)
 */
int execute_log_command(int index) {
    char commands[MAX_LOG_ENTRIES][4096];
    int count = read_all_commands(commands, MAX_LOG_ENTRIES);
    
    // Validate index range
    if (index < 1 || index > count) {
        return -1;
    }
    
    // Convert from 1-based newest-to-oldest index to array index
    // Index 1 = newest = commands[count-1]
    // Index 2 = second newest = commands[count-2]
    // Index N = oldest = commands[0]
    int array_index = count - index;
    
    // Get the command string from history
    char *command = commands[array_index];
    
    // Execute the command WITHOUT adding it to history
    // This prevents the executed command from appearing twice in history
    execute_command_without_logging(command);
    
    return 0;  // Success
}

/**
 * Main log command handler
 * 
 * This function parses and dispatches log command variants:
 * 
 * Usage patterns:
 * - "log"              → Display all commands in history (oldest to newest)
 * - "log purge"        → Clear all history 
 * - "log execute <N>"  → Execute command at index N (1-based, newest to oldest)
 * 
 * @param args: Array of command arguments (args[0] = "log")
 * @param arg_count: Number of arguments in the array
 * @return: 0 on success, -1 on error
 */
int log_command(char **args, int arg_count) {
    if (arg_count == 1) {
        // "log" with no arguments
        //  Display all commands in history
        print_log();
        return 0;
    }
    
    if (arg_count == 2 && strcmp(args[1], "purge") == 0) {
        // "log purge"
        // Clear the entire command history
        purge_log();
        return 0;
    }
    
    if (arg_count == 3 && strcmp(args[1], "execute") == 0) {
        // "log execute <index>"
        // Execute the command at the specified index
        
        // Parse the index argument with error checking
        char *endptr;
        long index = strtol(args[2], &endptr, 10);  // Convert string to long
        
        // Validate that entire string was consumed and index is positive
        if (*endptr != '\0' || index < 1) {
            return -1;
        }
        
        // Execute the command at the specified index
        return execute_log_command((int)index);
    }
    
    // If we reach here, the command syntax doesn't match any valid pattern
    // printf("Invalid Syntax!\n");
    return -1;
}
