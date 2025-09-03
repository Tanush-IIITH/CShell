#pragma once
#include "header.h"

/**
 * Sequential Command Execution Module
 * 
 * This module handles the semicolon operator (;) for executing multiple
 * commands sequentially. Commands are executed one after another, with
 * each command waiting for the previous to complete.
 * 
 * Syntax: command1 ; command2 ; ... ; commandN
 * 
 * Features:
 * - Sequential execution (one command at a time)
 * - Each command waits for previous to complete
 * - Commands are independent (no data sharing)
 * - Each command can be a full shell_cmd (pipes, redirection, etc.)
 * - Error in one command doesn't stop subsequent commands
 */

/**
 * Execute sequential commands separated by semicolons
 * 
 * Main function for handling semicolon-separated command sequences.
 * Parses the command string, splits on semicolons, and executes each
 * command in order.
 * 
 * @param command: Full command string containing semicolon operators
 */
void execute_sequential_commands(char *command);

/**
 * Check if a command string contains semicolon operators
 * 
 * Utility function to determine if a command needs sequential
 * execution handling.
 * 
 * @param command: Command string to check
 * @return: 1 if semicolons are present, 0 otherwise
 */
int contains_semicolon(const char *command);

/**
 * Count the number of commands in a semicolon-separated sequence
 * 
 * Utility function to count individual commands in a sequence.
 * 
 * @param command: Command string to analyze
 * @return: Number of individual commands
 */
int count_sequential_commands(const char *command);
