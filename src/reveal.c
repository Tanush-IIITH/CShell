#include "../include/header.h"
#include "../include/reveal.h"
#include "../include/hop.h"  // For accessing previous directory state

/**
 * Compare function for qsort to sort directory entries lexicographically
 * Uses ASCII values for comparison as required
 * 
 * @param a: Pointer to first string pointer (void* cast from char**)
 * @param b: Pointer to second string pointer (void* cast from char**)
 * @return: Negative if a < b, 0 if equal, positive if a > b
 */
int compare_entries(const void *a, const void *b) {
    // Cast void pointers back to char** and dereference to get actual strings
    const char *str_a = *(const char **)a;
    const char *str_b = *(const char **)b;
    
    // Use strcmp for lexicographic comparison based on ASCII values
    return strcmp(str_a, str_b);  // strcmp uses ASCII values for comparison
}

/**
 * Resolve the target directory path for reveal command
 * Follows the same logic as hop command for directory resolution
 * 
 * @param target: Target directory string (~ | . | .. | - | name)
 * @return: Resolved absolute path (caller must free), or NULL on error
 */
char* resolve_reveal_path(const char *target) {
    // Allocate memory for the resolved absolute path
    // PATH_MAX ensures we have enough space for any valid system path
    char *resolved_path = malloc(PATH_MAX);
    if (!resolved_path) {
        perror("malloc");  // Print system error message
        return NULL;
    }
    
    // Handle NULL target or current directory "."
    if (!target || strcmp(target, ".") == 0) {
        // Get current working directory using getcwd system call
        if (getcwd(resolved_path, PATH_MAX) == NULL) {
            perror("getcwd");  // Print error if getcwd fails
            free(resolved_path);  // Clean up allocated memory
            return NULL;
        }
    } 
    else if (strcmp(target, "~") == 0) {
        // Handle home directory resolution using system user database
        struct passwd *pw = getpwuid(getuid());  // Get user info from passwd database
        const char *home = pw ? pw->pw_dir : "/";  // Use home dir or fallback to root
        
        // Safely copy home directory path with bounds checking
        strncpy(resolved_path, home, PATH_MAX - 1);
        resolved_path[PATH_MAX - 1] = '\0';  // Ensure null termination
    } 
    else if (strcmp(target, "..") == 0) {
        // Handle parent directory resolution
        // First get the current working directory
        if (getcwd(resolved_path, PATH_MAX) == NULL) {
            perror("getcwd");  // Print error if getcwd fails
            free(resolved_path);  // Clean up allocated memory
            return NULL;
        }
        
        // Find the last occurrence of '/' to identify the parent directory
        // strrchr searches from the end of the string backwards
        char *last_slash = strrchr(resolved_path, '/');
        
        // Check if we found a slash and it's not the root directory
        if (last_slash && last_slash != resolved_path) {
            *last_slash = '\0';  // Truncate path at last slash to get parent
        } else {
            // If we're already at root ("/") or no slash found, stay at root
            strcpy(resolved_path, "/");  // Set to root directory
        }
    } 
    else if (strcmp(target, "-") == 0) {
        // Handle previous directory using hop command's directory history
        const char *prev_dir = get_previous_directory();  // Get from hop's state
        
        // Check if previous directory exists (non-empty string)
        if (strlen(prev_dir) == 0) {
            printf("No such directory!\n");  // No previous directory available
            free(resolved_path);  // Clean up allocated memory
            return NULL;
        }
        
        // Copy previous directory path with bounds checking
        strncpy(resolved_path, prev_dir, PATH_MAX - 1);
        resolved_path[PATH_MAX - 1] = '\0';  // Ensure null termination
    } else {
        // Handle named directory paths (absolute or relative)
        if (target[0] == '/') {
            // Absolute path: starts with '/', use as-is
            strncpy(resolved_path, target, PATH_MAX - 1);
            resolved_path[PATH_MAX - 1] = '\0';  // Ensure null termination
        } else {
            // Relative path: needs to be combined with current directory
            // First get the current working directory
            if (getcwd(resolved_path, PATH_MAX) == NULL) {
                perror("getcwd");  // Print error if getcwd fails
                free(resolved_path);  // Clean up allocated memory
                return NULL;
            }
            
            // Append the relative path to current directory
            size_t current_len = strlen(resolved_path);
            if (current_len < PATH_MAX - 1) {
                // Add separator '/' if current path doesn't end with one
                if (resolved_path[current_len - 1] != '/') {
                    strncat(resolved_path, "/", PATH_MAX - current_len - 1);
                }
                // Append the target relative path
                strncat(resolved_path, target, PATH_MAX - strlen(resolved_path) - 1);
            }
        }
    }
    
    return resolved_path;
}

/**
 * Parse reveal command flags and extract target directory
 * 
 * @param args: Command arguments array
 * @param arg_count: Number of arguments
 * @param flags: Pointer to flags structure to populate
 * @param target_dir: Pointer to store target directory (will point to args element)
 * @return: 0 on success, -1 on error
 */
int parse_reveal_flags(char **args, int arg_count, reveal_flags_t *flags, char **target_dir) {
    // Initialize flags structure to default values (all flags off)
    flags->show_hidden = 0;   // -a flag: show hidden files (starting with '.')
    flags->line_format = 0;   // -l flag: display one entry per line
    *target_dir = NULL;       // No target directory specified initially
    
    // Process arguments starting from index 1 (skip "reveal" command itself)
    for (int i = 1; i < arg_count; i++) {
        // Check if current argument is a flag (starts with '-' and has more characters)
        if (args[i][0] == '-' && strlen(args[i]) > 1) {
            // Process each character in the flag argument (supports combined flags like -al)
            for (int j = 1; args[i][j] != '\0'; j++) {
                switch (args[i][j]) {
                    case 'a':
                        flags->show_hidden = 1;  // Enable showing hidden files
                        break;
                    case 'l':
                        flags->line_format = 1;  // Enable line-by-line format
                        break;
                    default:
                        // Invalid flag character encountered
                        printf("Invalid Syntax!\n");
                        return -1;
                }
            }
        } else {
            // This argument is not a flag, treat as target directory
            // Check if we already have a target directory (only one allowed)
            if (*target_dir != NULL) {
                printf("Invalid Syntax!\n");  // Multiple directories specified
                return -1;
            }
            *target_dir = args[i];  // Store pointer to the directory argument
        }
    }
    
    return 0;  // Success
}

/**
 * List directory contents according to reveal command specifications
 * 
 * @param dir_path: Path to directory to list
 * @param flags: Flags controlling output format
 * @return: 0 on success, -1 on error
 */
int list_directory_contents(const char *dir_path, reveal_flags_t flags) {
    // Attempt to open the directory for reading
    DIR *dir = opendir(dir_path);
    if (!dir) {
        printf("Invalid Syntax!\n");  // Directory doesn't exist or no permission
        return -1;
    }
    
    /* FIRST PASS: Count valid entries to determine array size */
    struct dirent *entry;  // Structure to hold directory entry information
    int entry_count = 0;   // Counter for valid entries
    
    // Read through all directory entries to count them
    while ((entry = readdir(dir)) != NULL) {
        // Apply filtering based on flags
        if (!flags.show_hidden && entry->d_name[0] == '.') {
            continue;  // Skip hidden files unless -a flag is set
        }
        entry_count++;  // Count this valid entry
    }
    
    // Allocate array to store pointers to entry names
    char **entries = malloc(entry_count * sizeof(char*));
    if (!entries) {
        perror("malloc");  // Print system error message
        closedir(dir);     // Close directory before returning
        return -1;
    }
    
    /* SECOND PASS: Collect entry names into allocated array */
    rewinddir(dir);  // Reset directory stream to beginning
    int index = 0;   // Index for filling the entries array
    
    // Read through directory entries again to collect names
    while ((entry = readdir(dir)) != NULL && index < entry_count) {
        // Apply same filtering as first pass
        if (!flags.show_hidden && entry->d_name[0] == '.') {
            continue;  // Skip hidden files unless -a flag is set
        }
        
        // Create a copy of the entry name using strdup
        entries[index] = strdup(entry->d_name);
        if (!entries[index]) {
            perror("strdup");  // Print error if memory allocation fails
            
            // Clean up previously allocated entries to prevent memory leaks
            for (int i = 0; i < index; i++) {
                free(entries[i]);
            }
            free(entries);  // Free the entries array
            closedir(dir);  // Close directory
            return -1;
        }
        index++;  // Move to next array position
    }
    
    closedir(dir);  // Close directory as we're done reading it
    
    /* SORTING: Sort entries lexicographically using ASCII values */
    // qsort uses our compare_entries function to determine ordering
    qsort(entries, entry_count, sizeof(char*), compare_entries);
    
    /* OUTPUT: Display entries according to format flags */
    if (flags.line_format) {
        // Line format (-l flag): Display one entry per line
        for (int i = 0; i < entry_count; i++) {
            printf("%s\n", entries[i]);
        } 
    } else {
        // Default format: Display entries on same line separated by spaces
        for (int i = 0; i < entry_count; i++) {
            printf("%s", entries[i]);
            if (i < entry_count - 1) {
                printf("  ");  // Two spaces between entries for readability
            }
        }
        if (entry_count > 0) {
            printf("\n");  // Add final newline only if we printed something
        }
    }
    
    /* CLEANUP: Free all dynamically allocated memory */
    for (int i = 0; i < entry_count; i++) {
        free(entries[i]);  // Free each individual entry name
    }
    free(entries);  // Free the array of pointers
    
    return 0;  // Success
}

/**
 * Main reveal command handler
 * Implements the complete reveal command functionality
 * Syntax: reveal (-(a | l)*)* (~ | . |.. | - | name)?
 * 
 * @param args: Command arguments array (args[0] is "reveal")
 * @param arg_count: Total number of arguments including command name
 * @return: 0 on success, -1 on error
 */
int reveal_command(char **args, int arg_count) {
    reveal_flags_t flags;    // Structure to hold parsed flags
    char *target_dir = NULL; // Pointer to target directory argument
    
    /* STEP 1: Parse command line arguments */
    // Extract flags (-a, -l) and target directory from arguments
    if (parse_reveal_flags(args, arg_count, &flags, &target_dir) != 0) {
        return -1;  // Parsing failed (invalid syntax)
    }
    
    /* STEP 2: Resolve target directory to absolute path */
    // Convert special directories (~, ., .., -) and relative paths to absolute paths
    char *resolved_path = resolve_reveal_path(target_dir);
    if (!resolved_path) {
        return -1;  // Path resolution failed
    }
    
    /* STEP 3: List and display directory contents */
    // Read directory, filter, sort, and display according to flags
    int result = list_directory_contents(resolved_path, flags);
    
    /* STEP 4: Cleanup allocated memory */
    free(resolved_path);  // Free the resolved path memory
    
    return result;  // Return success/failure status
}
