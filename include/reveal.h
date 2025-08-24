#pragma once
#include "header.h"

/**
 * Reveal command for listing directory contents
 * Syntax: reveal (-(a | l)*)* (~ | . |.. | - | name)?
 * 
 * Flags:
 * -a: Show hidden files and directories (starting with .)
 * -l: Show one entry per line format
 * 
 * Directory argument follows same rules as hop command:
 * ~ : Home directory
 * . : Current directory  
 * .. : Parent directory
 * - : Previous directory
 * name : Named directory path
 */

// Structure to hold reveal command flags
typedef struct {
    int show_hidden;    // -a flag: show hidden files
    int line_format;    // -l flag: one entry per line
} reveal_flags_t;

// Function declarations

/**
 * Main reveal command handler - entry point for reveal command execution
 * Coordinates flag parsing, path resolution, and directory listing
 * @param args: Command arguments array (e.g., ["reveal", "-l", "/tmp"])
 * @param arg_count: Number of arguments in the array
 * @return: 0 on success, -1 on error
 */
int reveal_command(char **args, int arg_count);

/**
 * Parse reveal command arguments to extract flags and target directory
 * Processes flags like -a, -l and identifies the directory argument
 * @param args: Command arguments array
 * @param arg_count: Number of arguments
 * @param flags: Pointer to flags structure to populate
 * @param target_dir: Pointer to store target directory string
 * @return: 0 on success, -1 on invalid arguments
 */
int parse_reveal_flags(char **args, int arg_count, reveal_flags_t *flags, char **target_dir);

/**
 * List and display directory contents according to specified flags
 * Handles directory reading, sorting, and output formatting
 * @param dir_path: Absolute path to directory to list
 * @param flags: Formatting flags (show_hidden, line_format)
 * @return: 0 on success, -1 on error (e.g., directory not found)
 */
int list_directory_contents(const char *dir_path, reveal_flags_t flags);

/**
 * Comparator function for qsort to sort directory entries lexicographically
 * Used to ensure consistent alphabetical ordering of directory contents
 * @param a: Pointer to first string pointer
 * @param b: Pointer to second string pointer
 * @return: Negative, zero, or positive value for string comparison
 */
int compare_entries(const void *a, const void *b);

/**
 * Resolve symbolic directory references to absolute paths
 * Converts ~, ., .., -, and named paths to full directory paths
 * @param target: Symbolic directory reference (can be NULL for current dir)
 * @return: Dynamically allocated absolute path string (caller must free), or NULL on error
 */
char* resolve_reveal_path(const char *target);
