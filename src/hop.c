#include "../include/header.h"
#include "../include/hop.h"

/* ===============================================
 * GLOBAL STATE VARIABLES FOR HOP COMMAND
 * =============================================== */

// Static variables maintain state across function calls within this file only
static char previous_directory[4096] = "";  // Stores the last directory for 'hop -' command
static char home_directory[4096] = "";      // Stores user's home directory for 'hop ~' and 'hop' commands  
static int hop_initialized = 0;             // Flag to prevent re-initialization (0=not initialized, 1=initialized)

/* ===============================================
 * INITIALIZATION FUNCTION
 * =============================================== */

/**
 * Initialize hop command state - called once during shell session
 * This function sets up the home directory path and initializes tracking variables
 * Uses system calls to get user information and safely stores directory paths
 */
void init_hop_state(void) {
    /* Step 1: Get user information from system password database */
    struct passwd *pw = getpwuid(getuid());
    // getuid() returns current user's UID, getpwuid() looks up user info in /etc/passwd
    
    /* Step 2: Extract and validate home directory path */
    if (pw && pw->pw_dir) {
        // Both user lookup and home directory field are valid
        // Use strncpy for safe copying to prevent buffer overflow
        strncpy(home_directory, pw->pw_dir, sizeof(home_directory) - 1);
        
        // Manually ensure null termination (strncpy doesn't guarantee this)
        // This is critical for string safety - always null-terminate manually
        home_directory[sizeof(home_directory) - 1] = '\0';
    } else {
        // Fallback: user lookup failed or home directory not found
        // Use root directory as last resort (guaranteed to exist on any Unix system)
        strcpy(home_directory, "/"); 
    }
    
    /* Step 3: Initialize directory history tracking */
    // Set previous directory to empty string - no history exists yet
    // This ensures 'hop -' will do nothing until after first directory change
    previous_directory[0] = '\0';
    
    /* Step 4: Mark initialization as complete */
    // Prevents this expensive initialization from running multiple times
    // Critical for preserving directory history across hop command calls
    hop_initialized = 1;
}

/* ===============================================
 * PUBLIC ACCESSOR FUNCTION
 * =============================================== */

/**
 * Get home directory path with lazy initialization
 * This function provides safe access to the home directory path
 * Automatically initializes if not already done (lazy initialization pattern)
 * 
 * @return Pointer to home directory string (statically allocated, safe to use)
 */
char* get_home_directory(void) {
    // Check if initialization has been performed
    if (!hop_initialized) {
        // First call - perform initialization
        init_hop_state();
    }
    
    // Return pointer to statically allocated home directory string
    // Safe to use - string persists for entire program lifetime
    return home_directory;
}

/* ===============================================
 * DIRECTORY HISTORY MANAGEMENT
 * =============================================== */

/**
 * Save current working directory as "previous" for 'hop -' command
 * This function must be called before changing directories to maintain history
 * Uses getcwd() to get current directory and safely stores it
 * Critical for enabling 'hop -' functionality
 */
static void save_current_as_previous(void) {
    // Get current working directory (dynamically allocated by getcwd)
    // getcwd(NULL, 0) allocates appropriate buffer size automatically
    char *current = getcwd(NULL, 0);
    
    if (current) {
        // Successfully got current directory - save it safely
        
        // Use strncpy to prevent buffer overflow when copying directory path
        strncpy(previous_directory, current, sizeof(previous_directory) - 1);
        
        // Manually ensure null termination for string safety
        // This guarantees previous_directory is a valid C string
        previous_directory[sizeof(previous_directory) - 1] = '\0';
        
        // Free dynamically allocated memory from getcwd()
        // Critical: getcwd(NULL, 0) allocates memory that we must free
        free(current);
    }
    // If getcwd() failed, previous_directory remains unchanged
    // This preserves existing history rather than corrupting it
}

/* ===============================================
 * DIRECTORY CHANGE OPERATION
 * =============================================== */

/**
 * Change directory with comprehensive error handling and feedback
 * This function performs the actual directory change using chdir() system call
 * Provides user feedback and handles errors gracefully
 * 
 * @param path Directory path to change to (relative or absolute)
 * @return 1 on success, 0 on failure
 */
static int change_directory(const char *path) {
    // Capture current directory before attempting change (for debugging/feedback)
    char *before = getcwd(NULL, 0);
    
    // Attempt to change directory using chdir() system call
    if (chdir(path) == 0) {
        /* SUCCESS PATH: Directory change succeeded */
        
        // Get new current directory for confirmation message
        char *after = getcwd(NULL, 0);
        
        // Clean up dynamically allocated memory from getcwd() calls
        if (before) free(before);
        if (after) free(after);
        
        return 1; // Success - directory change completed
        
    } else {
        /* FAILURE PATH: Directory change failed */
        
        // Inform user about the failure with specific path that caused error
        printf("No such directory: %s\n", path);
        
        // Clean up memory (before allocation may have succeeded even if chdir failed)
        if (before) free(before);
        
        return 0; // Failure - directory change failed
    }
}

/* ===============================================
 * MAIN HOP COMMAND IMPLEMENTATION
 * =============================================== */

/**
 * Main hop command processor - implements the grammar: hop ((~ | . | .. | - | name)*)?
 * This function handles all hop command variations and processes multiple arguments sequentially
 * 
 * @param args Array of command arguments (args[0] = "hop", args[1..n] = hop arguments)
 * @param arg_count Total number of arguments including command name
 * @return 1 on success, 0 on failure (stops on first error)
 */
int hop_command(char **args, int arg_count) {
    /* Step 1: Ensure initialization has been performed */
    if (!hop_initialized) {
        // First hop command call - initialize directory tracking state
        init_hop_state();
    }
    
    /* Step 2: Handle no-argument case - 'hop' alone goes to home directory */
    if (arg_count == 1) { 
        // Only "hop" command provided, no additional arguments
        // According to grammar: hop goes to home directory
        
        save_current_as_previous();              // Preserve current location for 'hop -'
        return change_directory(home_directory); // Change to home and return result
    }
    
    /* Step 3: Process each argument sequentially (left to right) */
    // Grammar allows multiple arguments: hop arg1 arg2 arg3 ...
    // Each argument is processed in order, building a navigation sequence
    for (int i = 1; i < arg_count; i++) {
        char *arg = args[i];  // Current argument being processed
        
        /* Handle special argument: ~ (tilde - home directory) */
        if (strcmp(arg, "~") == 0) {
            // Grammar rule: ~ -> go to home directory
            
            save_current_as_previous();              // Save current for history
            if (!change_directory(home_directory)) { // Attempt directory change
                return 0; // Stop processing on first error - fail-fast behavior
            }
            
        /* Handle special argument: . (dot - current directory) */    
        } else if (strcmp(arg, ".") == 0) {
            // Grammar rule: . -> stay in current directory (no operation)
            // Use continue to skip to next argument without any action
            continue;
            
        /* Handle special argument: .. (double dot - parent directory) */
        } else if (strcmp(arg, "..") == 0) {
            // Grammar rule: .. -> go to parent directory
            
            save_current_as_previous();        // Save current for history
            if (!change_directory("..")) {     // Use ".." path for parent
                return 0; // Stop processing on first error
            }
            
        /* Handle special argument: - (dash - previous directory) */
        } else if (strcmp(arg, "-") == 0) {
            // Grammar rule: - -> go to previous directory (if exists)
            
            // Check if previous directory exists in our history
            if (previous_directory[0] == '\0') {
                // No previous directory recorded - do nothing
                // This happens: 1) At shell startup, 2) After hop commands that didn't save history
                continue; // Skip to next argument
            }
            
            // We need to go to old previous, but also update previous to current
            // This enables repeated 'hop -' to toggle between two directories
            
            // Step 1: Save old previous directory before we overwrite it
            char temp_prev[4096];
            strncpy(temp_prev, previous_directory, sizeof(temp_prev) - 1);
            temp_prev[sizeof(temp_prev) - 1] = '\0';  // Ensure null termination
            
            // Step 2: Update previous_directory to current directory
            save_current_as_previous(); // Now previous_directory = current
            
            // Step 3: Change to the old previous directory (saved in temp_prev)
            if (!change_directory(temp_prev)) {
                return 0; // Stop processing on first error
            }
            
        /* Handle default case: name (regular directory path) */
        } else {
            // Grammar rule: name -> any directory path (relative or absolute)
            // Examples: "Documents", "/tmp", "../parent", "/home/user/folder"
            
            save_current_as_previous();    // Save current for history
            if (!change_directory(arg)) {  // Use argument as directory path
                return 0; // Stop processing on first error
            }
        }
    } // End of argument processing loop
    
    /* Step 4: All arguments processed successfully */
    return 1; // Success - all directory changes completed without errors
}
