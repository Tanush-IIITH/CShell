#pragma once
#include "header.h"

// Function declarations for command execution
void execute_command(char *command);
void execute_command_without_logging(char *command);
char** parse_command_args(char *command, int *arg_count, char **input_file, char **output_file, int *append_mode);
void free_command_args(char **args, int arg_count);
int setup_input_redirection(const char *input_file);
int setup_output_redirection(const char *output_file, int append_mode);
