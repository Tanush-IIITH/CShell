#pragma once
#include "header.h"

// Function declarations for command execution
void execute_command(char *command);
char** parse_command_args(char *command, int *arg_count);
void free_command_args(char **args, int arg_count);
