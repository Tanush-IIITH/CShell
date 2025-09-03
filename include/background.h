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
#define MAX_BACKGROUND_JOBS 100

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
