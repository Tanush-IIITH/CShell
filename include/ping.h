#pragma once

#include "header.h"

/**
 * Execute the ping command to send signal to a process
 * 
 * Syntax: ping <pid> <signal_number>
 * 
 * The command sends a signal to a process with the specified PID.
 * The signal number is taken modulo 32 before sending.
 * 
 * @param args: Command arguments ["ping", "<pid>", "<signal_number>"]
 * @param arg_count: Number of arguments (should be 3)
 * @return: 0 on success, -1 on error
 * 
 * Output messages:
 * - Success: "Sent signal <signal_number> to process with pid <pid>"
 * - Process not found: "No such process found"
 * - Invalid arguments: Error message about usage
 */
int execute_ping_command(char **args, int arg_count);

/**
 * Send a signal to a process and handle the response
 * 
 * @param pid: Process ID to send signal to
 * @param signal_number: Signal number to send (will be taken modulo 32)
 * @return: 0 on success, -1 on error
 */
int send_signal_to_process(pid_t pid, int signal_number);

/**
 * Validate if a PID exists in our activities tracking
 * 
 * @param pid: Process ID to check
 * @return: 1 if process exists in activities, 0 if not found
 */
int is_tracked_process(pid_t pid);
