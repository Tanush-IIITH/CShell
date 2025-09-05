#pragma once

#include "header.h"

/**
 * Initialize signal handling for the shell
 * Sets up handlers for SIGINT to prevent shell termination
 */
void init_signal_handling(void);

/**
 * Set the current foreground process PID (for single commands)
 * Called when a foreground process starts
 * 
 * @param pid: Process ID of the foreground process (0 if no foreground process)
 */
void set_foreground_process(pid_t pid);

/**
 * Set the current foreground process group (for pipelines)
 * Called when a pipeline starts - signals entire group on Ctrl-C
 * 
 * @param pgid: Process Group ID of the foreground pipeline (0 if no foreground group)
 */
void set_foreground_process_group(pid_t pgid);

/**
 * Get the current foreground process PID
 * 
 * @return: PID of current foreground process, or 0 if none
 */
pid_t get_foreground_process(void);
