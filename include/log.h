#pragma once

#include "header.h"

#define MAX_LOG_ENTRIES 15
#define LOG_FILE ".myshell_history"

/**
 * Add a command to the log
 * @param command: The full command string to store
 * @return: 0 on success, -1 on error
 */
int add_to_log(const char *command);

/**
 * Main log command handler
 * @param args: Command arguments array
 * @param arg_count: Number of arguments
 * @return: 0 on success, -1 on error
 */
int log_command(char **args, int arg_count);

/**
 * Clear the log (purge operation)
 */
void purge_log();

/**
 * Execute a command from log by index
 * @param index: 1-based index (newest to oldest)
 * @return: 0 on success, -1 on error
 */
int execute_log_command(int index);
