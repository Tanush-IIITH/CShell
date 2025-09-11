#pragma once
#include "header.h"

void init_shell_home(void);
char* get_shell_home_directory(void);

// Function declarations for shell prompt functionality
char* get_username(void); //gets the username from environment variable or system call
char* get_hostname(void); //gets the hostname from environment variable or system call
char* get_current_path(void); //gets the current working directory
void display_shell_prompt(void); //displays the shell prompt