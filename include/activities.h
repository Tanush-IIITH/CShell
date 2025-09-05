#pragma once

#include "header.h"

// Maximum number of processes we can track
#define MAX_ACTIVITIES 1000

// Process states
typedef enum {
    PROC_RUNNING,
    PROC_STOPPED,
    PROC_TERMINATED  // Used internally, not displayed
} ProcessState;

// Structure to track a single process
typedef struct {
    pid_t pid;                    // Process ID
    char command_name[256];       // Command name (first word of command)
    char full_command[1024];      // Full command for debugging/future features (not currently displayed)
    ProcessState state;           // Current state
    int is_active;               // 1 if slot is in use, 0 if free
    int is_background;           // 1 if background process, 0 if foreground
} ActivityEntry;

// Global activities tracking
extern ActivityEntry activities[MAX_ACTIVITIES];
extern int activity_count;

/**
 * Initialize the activities tracking system
 */
void init_activities(void);

/**
 * Add a new process to activities tracking
 * @param pid: Process ID
 * @param command: Full command string
 * @param is_background: 1 if background process, 0 if foreground
 * @return: 0 on success, -1 on failure
 */
int add_activity(pid_t pid, const char *command, int is_background);

/**
 * Remove a process from activities tracking
 * @param pid: Process ID to remove
 * @return: 0 on success, -1 if not found
 */
int remove_activity(pid_t pid);

/**
 * Update the state of a process
 * @param pid: Process ID
 * @param new_state: New state (PROC_RUNNING, PROC_STOPPED)
 * @return: 0 on success, -1 if not found
 */
int update_activity_state(pid_t pid, ProcessState new_state);

/**
 * Check all tracked processes and update their states
 * Removes terminated processes from tracking
 */
void update_all_activities(void);

/**
 * Display all active processes sorted by command name
 * Format: [pid] : command_name - State
 */
void display_activities(void);

/**
 * Execute the activities command
 * @param args: Command arguments (should just be ["activities"])
 * @param arg_count: Number of arguments
 * @return: 0 on success, -1 on error
 */
int execute_activities_command(char **args, int arg_count);

/**
 * Extract command name from full command string
 * @param command: Full command string
 * @param cmd_name: Buffer to store extracted command name
 * @param max_len: Maximum length of cmd_name buffer
 */
void extract_command_name(const char *command, char *cmd_name, size_t max_len);

/**
 * Get the current state of a process
 * @param pid: Process ID to check
 * @return: ProcessState (PROC_RUNNING, PROC_STOPPED, PROC_TERMINATED)
 */
ProcessState get_process_state(pid_t pid);

/**
 * Convert ProcessState enum to string
 * @param state: ProcessState enum value
 * @return: String representation ("Running", "Stopped")
 */
const char* state_to_string(ProcessState state);

/**
 * Find activity entry by PID
 * @param pid: Process ID to find
 * @return: Pointer to ActivityEntry or NULL if not found
 */
ActivityEntry* find_activity(pid_t pid);

/**
 * Clean up terminated processes from activities list
 */
void cleanup_terminated_activities(void);
