#pragma once
#include "header.h"

// Function declarations for command execution
void execute_command(char *command);
void execute_command_without_logging(char *command);
char** parse_command_args(char *command, int *arg_count, char **input_file);
void free_command_args(char **args, int arg_count);
