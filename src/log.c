#include "../include/header.h"
#include "../include/log.h"
#include "../include/command.h"

/*
 * LOG SYSTEM OVERVIEW:
 * 
 * This module implements a persistent command history system with the following features:
 * 1. Circular buffer storage (fixed size: MAX_LOG_ENTRIES = 15)
 * 2. File persistence across shell sessions (stored in shell project folder)
 * 3. Duplicate command prevention
 * 4. Command execution from history by index
 * 5. History purging capability
 * 
 * The circular buffer ensures that when the history is full, new commands
 * automatically overwrite the oldest entries, maintaining a rolling window
 * of the most recent commands.
 */

// Global variables for log management - these maintain the state of our circular buffer
static LogEntry log_entries[MAX_LOG_ENTRIES];  // The circular buffer array
static int log_count = 0;        // Current number of entries (0 to MAX_LOG_ENTRIES)
static int log_start = 0;        // Index of oldest entry in circular buffer
static int log_initialized = 0;  // Prevents multiple initializations

/**
 * Get the log file path (in shell project folder)
 * 
 * Constructs the path to the history file in the shell project folder.
 * This ensures the log file is always stored with the shell project files,
 * regardless of where the shell executable is run from.
 * 
 * @return: Static string containing the full path to the log file
 *          The returned pointer is valid until the next call to this function
 */
static char* get_log_file_path() {
    static char log_path[PATH_MAX];  // Static storage for the path string
    
    // Store log file in the shell project folder
    // Use direct filename (will be created in current working directory)
    snprintf(log_path, sizeof(log_path), "%s", LOG_FILE);
    return log_path;
}

/**
 * Initialize the log system
 * 
 * This function sets up the log system for use. It's designed to be called
 * once at shell startup and is idempotent (safe to call multiple times).
 * 
 * Initialization process:
 * 1. Reset circular buffer pointers to empty state
 * 2. Load any existing history from persistent storage
 * 3. Mark system as initialized to prevent re-initialization
 */
void init_log() {
    if (log_initialized) return;  // Already initialized, nothing to do
    
    // Reset circular buffer to empty state
    log_count = 0;  // No entries initially
    log_start = 0;  // Start at beginning of buffer
    
    // Load any existing history from file
    load_log();
    
    // Mark as initialized to prevent multiple calls
    log_initialized = 1;
}

/**
 * Load log from file
 * 
 * Reads command history from persistent storage and populates the circular buffer.
 * This function is called during initialization to restore history from previous
 * shell sessions.
 * 
 * File format: One command per line, stored in chronological order
 * 
 * Behavior:
 * - If file doesn't exist, starts with empty history (normal for first run)
 * - Reads up to MAX_LOG_ENTRIES commands
 * - Older commands beyond the limit are ignored (keeps most recent)
 * - Handles malformed lines gracefully by skipping them
 */
void load_log() {
    FILE *file = fopen(get_log_file_path(), "r");
    if (!file) {
        // File doesn't exist - this is normal for first run
        // Start with empty log, no error message needed
        return;
    }
    
    char line[4096];  // Buffer for reading each line from file
    log_count = 0;    // Reset counters for fresh load
    log_start = 0;
    
    // Read lines until EOF or buffer full
    while (fgets(line, sizeof(line), file) && log_count < MAX_LOG_ENTRIES) {
        // Clean up the line: remove trailing newline character
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';  // Replace newline with null terminator
        }
        
        // Store command in circular buffer at next available position
        strncpy(log_entries[log_count].command, line, sizeof(log_entries[log_count].command) - 1);
        log_entries[log_count].command[sizeof(log_entries[log_count].command) - 1] = '\0';  // Ensure null termination
        log_count++;
    }
    
    fclose(file);
}

/**
 * Save the current log to file
 * 
 * Writes the entire command history to persistent storage. This function
 * is called after every command addition to ensure history survives shell
 * crashes or unexpected termination.
 * 
 * Writing strategy:
 * - Writes commands in chronological order (oldest to newest)
 * - Handles circular buffer wraparound correctly
 * - Overwrites entire file each time (simple but reliable)
 * - Prints error message if file operations fail
 */
void save_log() {
    FILE *file = fopen(get_log_file_path(), "w");
    if (!file) {
        perror("Failed to save log");  // Let user know about persistence failure
        return;
    }
    
    // Write entries in chronological order (oldest to newest)
    // This requires careful handling of the circular buffer indices
    for (int i = 0; i < log_count; i++) {
        // Calculate actual array index, accounting for circular buffer wraparound
        int index = (log_start + i) % MAX_LOG_ENTRIES;
        fprintf(file, "%s\n", log_entries[index].command);
    }
    
    fclose(file);
}

/**
 * Check if command should be logged
 * 
 * Implements filtering logic to decide whether a command should be added
 * to the history. This prevents cluttering the history with unwanted entries.
 * 
 * Commands are NOT logged if:
 * 1. Command is empty or whitespace-only
 * 2. Command starts with "log" (prevents log commands from appearing in history)
 * 3. Command is identical to the immediately previous command (prevents duplicates)
 * 
 * @param command: The command string to evaluate
 * @return: 1 if command should be logged, 0 if it should be filtered out
 */
static int should_log_command(const char *command) {
    // Filter out empty or null commands
    if (!command || strlen(command) == 0) {
        return 0;  // Nothing to log
    }
    
    // Parse the command to extract the first word (command name)
    char *cmd_copy = strdup(command);  // Create modifiable copy for strtok
    char *token = strtok(cmd_copy, " \t\n\r");  // Extract first token
    
    // Don't log "log" commands themselves (prevents meta-commands in history)
    if (token && strcmp(token, "log") == 0) {
        free(cmd_copy);
        return 0;  // Filter out log commands
    }
    
    free(cmd_copy);  // Clean up temporary string
    
    // Prevent duplicate consecutive commands from cluttering history
    if (log_count > 0) {
        // Calculate index of most recent command in circular buffer
        int last_index = (log_start + log_count - 1) % MAX_LOG_ENTRIES;
        
        // Compare with the last logged command
        if (strcmp(log_entries[last_index].command, command) == 0) {
            return 0;  // Duplicate of last command, don't log
        }
    }
    
    return 1;  // Command should be logged
}

/**
 * Add a command to the log
 * 
 * This is the main entry point for adding commands to the history.
 * It handles the complex logic of circular buffer management and
 * ensures the history is persistently stored.
 * 
 * Circular Buffer Logic:
 * - If buffer not full: append at end, increment count
 * - If buffer is full: overwrite oldest entry, advance start pointer
 * 
 * @param command: The command string to add to history
 * @return: 0 on success, non-zero on error
 */
int add_to_log(const char *command) {
    // Ensure log system is initialized before use
    if (!log_initialized) {
        init_log();
    }
    
    // Check if this command should be logged (filtering logic)
    if (!should_log_command(command)) {
        return 0;  // Command filtered out, but this is not an error
    }
    
    int insert_index;  // Where to place the new command
    
    if (log_count < MAX_LOG_ENTRIES) {
        // Buffer still has space - simple case
        insert_index = log_count;  // Insert at end
        log_count++;               // Increment total count
    } else {
        // Buffer is full - circular buffer case
        // Overwrite the oldest entry and advance the start pointer
        insert_index = log_start;                           // Overwrite oldest
        log_start = (log_start + 1) % MAX_LOG_ENTRIES;     // Advance start (with wraparound)
    }
    
    // Store the command in the calculated position
    strncpy(log_entries[insert_index].command, command, sizeof(log_entries[insert_index].command) - 1);
    log_entries[insert_index].command[sizeof(log_entries[insert_index].command) - 1] = '\0';  // Ensure null termination
    
    // Persist the updated history to disk immediately
    save_log();
    
    return 0;  // Success
}

/**
 * Clear the log (purge operation)
 * 
 * Removes all commands from the history and updates persistent storage.
 * This implements the "log purge" command functionality.
 * 
 * After purging:
 * - In-memory history is empty
 * - History file is empty
 * - Next command will be index 1
 */
void purge_log() {
    log_count = 0;  // Reset to empty state
    log_start = 0;  // Reset start pointer
    
    // Explicitly create/truncate the log file to ensure it's empty
    FILE *file = fopen(get_log_file_path(), "w");
    if (file) {
        fclose(file);  // Create empty file
    } else {
        perror("Failed to purge log file");
    }
}

/**
 * Print all log entries (oldest to newest)
 * 
 * Displays the command history with 1-based indexing for user reference.
 * The index numbers shown correspond to those used by "log execute <index>".
 * 
 * Display format: "1. first_command\n2. second_command\n..."
 * 
 * Index numbering:
 * - Index 1 = oldest command in history
 * - Index N = newest command in history (where N = log_count)
 */
static void print_log() {
    if (log_count == 0) {
        return;
    }
    
    // Iterate through history in chronological order (oldest to newest)
    for (int i = 0; i < log_count; i++) {
        // Calculate actual array index, handling circular buffer wraparound
        int index = (log_start + i) % MAX_LOG_ENTRIES;
        
        // Print with 1-based indexing for user friendliness
        printf("%s\n",log_entries[index].command);
    }
}

/**
 * Execute a command from log by index (1-based indexing)
 * 
 * This function implements the "log execute <index>" functionality.
 * It retrieves a command from history and executes it without adding
 * the executed command back to the history (prevents duplication).
 * 
 * Index interpretation:
 * - Index 1 = oldest command in current history
 * - Index N = newest command in current history
 * 
 * The function prints the command before executing it so the user
 * can see what's being run.
 * 
 * @param index: 1-based index of command to execute
 * @return: 0 on success, -1 on error (invalid index)
 */
int execute_log_command(int index) {
    // Validate index range
    if (index < 1 || index > log_count) {
        printf("log: Invalid index\n");
        return -1;
    }
    
    // Convert from 1-based user index to 0-based array index
    // Note: index 1 corresponds to the oldest command (log_start + 0)
    //       index N corresponds to the newest command (log_start + log_count - 1)
    int relative_index = index - 1;  // Convert to 0-based
    int actual_index = (log_start + relative_index) % MAX_LOG_ENTRIES;
    
    // Get the command string from history
    char *command = log_entries[actual_index].command;
    
    // Show user what command is being executed
    printf("%s\n", command);
    
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
 * - "log execute <N>"  → Execute command at index N (1-based)
 * 
 * Error handling:
 * - Invalid syntax shows "Invalid Syntax!" message
 * - Invalid indices show "log: Invalid index" message
 * - Non-numeric indices are rejected
 * 
 * @param args: Array of command arguments (args[0] = "log")
 * @param arg_count: Number of arguments in the array
 * @return: 0 on success, -1 on error
 */
int log_command(char **args, int arg_count) {
    // Ensure log system is ready for use
    if (!log_initialized) {
        init_log();
    }
    
    if (arg_count == 1) {
        // Case: "log" with no arguments
        // Action: Display all commands in history
        print_log();
        return 0;
    }
    
    if (arg_count == 2 && strcmp(args[1], "purge") == 0) {
        // Case: "log purge"
        // Action: Clear the entire command history
        purge_log();
        return 0;
    }
    
    if (arg_count == 3 && strcmp(args[1], "execute") == 0) {
        // Case: "log execute <index>"
        // Action: Execute the command at the specified index
        
        // Parse the index argument with error checking
        char *endptr;
        long index = strtol(args[2], &endptr, 10);  // Convert string to long
        
        // Validate that entire string was consumed and index is positive
        if (*endptr != '\0' || index < 1) {
            printf("log: Invalid index\n");
            return -1;
        }
        
        // Execute the command at the specified index
        return execute_log_command((int)index);
    }
    
    // If we reach here, the command syntax doesn't match any valid pattern
    printf("Invalid Syntax!\n");
    return -1;
}
