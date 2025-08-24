#pragma once
#include "header.h"

// Function declarations for hop command (directory navigation)
int hop_command(char **args, int arg_count);
void init_hop_state(void);
char* get_home_directory(void);
const char* get_previous_directory(void);  // Get previous directory for reveal command
