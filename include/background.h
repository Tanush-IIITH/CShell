#pragma once
#include "header.h"

/**
 * Background Process Management Module
 * 
 * This module handles the ampersand operator (&) for executing commands
 * in the background while the shell continues to accept new input.
 * 
 * Syntax: command &
 * 
 * Features:
 * - Fork process without waiting for completion
 * - Track background jobs with job numbers and PIDs
 * - Monitor and report when background processes complete
 * - Handle process exit status reporting
 * - Prevent background processes from accessing terminal input
 */

// Maximum number of background jobs that can be tracked simultaneously
#define MAX_BACKGROUND_JOBS 4194400

// Structure to track background job information
typedef struct {
    int job_number;          // Sequential job number (1, 2, 3, ...)
    pid_t pid;              // Process ID of the background job
    char *command_name;     // Name of the command for reporting
    int is_active;          // 1 if job is still running, 0 if completed
} background_job_t;

/**
 * Initialize the background job management system
 * 
 * Sets up data structures and signal handlers needed for
 * tracking background processes.
 */
void init_background_jobs(void);

/**
 * Check if a command string contains the background operator
 * 
 * Determines if a command should be executed in the background
 * by checking for the & operator at the end.
 * 
 * @param command: Command string to check
 * @return: 1 if command ends with &, 0 otherwise
 */
int contains_background_operator(const char *command);

/**
 * Check if command contains sequential background commands (multiple &)
 * 
 * Examples: "cmd1 & cmd2 & cmd3", "sleep 1 & echo hello & ls"
 * 
 * @param command: Command string to check
 * @return: 1 if command contains multiple & separated commands, 0 otherwise
 */
int contains_sequential_background_commands(const char *command);

/**
 * Execute a command in the background
 * 
 * Forks a child process to run the command without waiting for
 * completion. Registers the job and prints job information.
 * 
 * @param command: Command string (with & removed)
 * @return: 0 on success, -1 on error
 */
int execute_background_command(char *command);

/**
 * Execute a single command in the background
 * 
 * Creates a new process to run the command and tracks it in the
 * background jobs list.
 * 
 * @param command: Command string to execute (without & operator)
 * @return: 0 on success, -1 on error
 */
int execute_background_command(char *command);

/**
 * Execute sequential background commands separated by &
 * 
 * Parses and executes multiple commands like "cmd1 & cmd2 & cmd3"
 * The last command is executed in foreground unless the original string ends with &
 * 
 * Examples:
 * - "cmd1 & cmd2 & cmd3 &" → All 3 commands in background
 * - "cmd1 & cmd2 & cmd3" → cmd1,cmd2 in background, cmd3 in foreground
 * 
 * @param command: Command string containing multiple & operators
 */
void execute_sequential_background_commands(char *command);

/**
 * Check for completed background jobs
 * 
 * Should be called before processing each new input to check
 * if any background processes have completed and report their status.
 */
void check_background_jobs(void);

/**
 * Clean up background job tracking
 * 
 * Frees resources and cleans up when shell exits.
 */
void cleanup_background_jobs(void);

/**
 * Remove the background operator from a command string
 * 
 * Strips the trailing & and any surrounding whitespace.
 * 
 * @param command: Command string to modify (modified in-place)
 */
void remove_background_operator(char *command);

/**
 * Add a stopped process to background jobs
 * 
 * Registers a process that was stopped via Ctrl-Z as a background job
 * 
 * @param pid: Process ID of the stopped process
 * @param command_name: Name of the command for display
 * @param is_stopped: 1 if process is stopped, 0 if running
 * @return: Job number assigned, or -1 on error
 */
int add_stopped_job(pid_t pid, const char *command_name, int is_stopped);

/**
 * Bring a job to the foreground
 * 
 * @param job_number: Job number to bring to foreground (-1 for most recent)
 * @return: 0 on success, -1 on error
 */
int fg_command(int job_number);

/**
 * Resume a job in the background
 * 
 * @param job_number: Job number to resume (-1 for most recent)
 * @return: 0 on success, -1 on error
 */
int bg_command(int job_number);
