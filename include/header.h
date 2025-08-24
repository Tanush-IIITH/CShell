#pragma once //prevents header file from being included multiple times

// Feature test macros for POSIX functions
#define _POSIX_C_SOURCE 200112L //enables POSIX.1-2001 features
#define _DEFAULT_SOURCE //enables default features for POSIX and C standard library

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>    // For directory operations (opendir, readdir, closedir)
#include <limits.h>    // For PATH_MAX constant