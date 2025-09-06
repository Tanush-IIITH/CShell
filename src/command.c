#include "header.h"
#include "sequential.h"
#include "background.h"
#include "../include/command.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/log.h"
#include "../include/activities.h"
#include "../include/ping.h"
#include "../include/signal_handler.h"

/**
 * Parse command string into arguments array with input and output redirection support
 * 
 * This function takes a command string (e.g., "hop /tmp .." or "cat < file.txt > output.txt") 
 * and breaks it down into individual arguments that can be processed by command handlers.
 * It handles input redirection with the < operator and output redirection with > and >> operators.
 * 
 * @param command: Input command string to be parsed (will be modified by strtok)
 * @param arg_count: Pointer to integer that will store the number of arguments found
 * @param input_file: Pointer to string pointer that will store input redirection file (or NULL)
 * @param output_file: Pointer to string pointer that will store output redirection file (or NULL)
 * @param append_mode: Pointer to int that will store 1 for >>, 0 for >, -1 for no output redirection
 * @return: Array of string pointers containing the parsed arguments, NULL-terminated
 * 
 * Memory allocation: Caller is responsible for freeing the returned array and its contents
 * using free_command_args(), and freeing input_file and output_file if not NULL
 */
char** parse_command_args(char *command, int *arg_count, char **input_file, char **output_file, int *append_mode) {
    // Allocate memory for argument pointers array (support up to 64 arguments)
    // Each element will point to a dynamically allocated string
    char **args = malloc(64 * sizeof(char*));
    *arg_count = 0;  // Initialize argument counter
    *input_file = NULL;  // Initialize input file pointer
    *output_file = NULL;  // Initialize output file pointer
    *append_mode = -1;   // Initialize append mode (-1 = no output redirection)
    
    // Clean up the command string by removing trailing newline character
    // This is necessary because input from fgets() often includes '\n'
    char *newline = strchr(command, '\n');
    if (newline) {
        *newline = '\0';  // Replace newline with null terminator
    }
    
    // Begin tokenization process using strtok()
    // Split on space, tab, newline, and carriage return characters
    char *token = strtok(command, " \t\n\r");
    
    // Process each token until we reach the end or hit our argument limit
    while (token != NULL && *arg_count < 63) { // Reserve space for NULL terminator
        if (strcmp(token, "<") == 0) {
            // Input redirection operator found
            token = strtok(NULL, " \t\n\r");  // Get the filename
            if (token != NULL) {
                // If multiple input redirections, only keep the last one
                if (*input_file) {
                    free(*input_file);
                }
                *input_file = strdup(token);
            }
        } else if (strcmp(token, ">") == 0) {
            // Output redirection operator found (overwrite mode)
            token = strtok(NULL, " \t\n\r");  // Get the filename
            if (token != NULL) {
                // If multiple output redirections, only keep the last one
                if (*output_file) {
                    free(*output_file);
                }
                *output_file = strdup(token);
                *append_mode = 0;  // 0 = overwrite mode
            }
        } else if (strcmp(token, ">>") == 0) {
            // Output redirection operator found (append mode)
            token = strtok(NULL, " \t\n\r");  // Get the filename
            if (token != NULL) {
                // If multiple output redirections, only keep the last one
                if (*output_file) {
                    free(*output_file);
                }
                *output_file = strdup(token);
                *append_mode = 1;  // 1 = append mode
            }
        } else {
            // Regular argument - create a permanent copy of the token using strdup()
            // This is necessary because strtok() returns pointers to the original string
            args[*arg_count] = strdup(token);
            (*arg_count)++;  // Increment argument counter
        }
        
        // Get the next token from the remaining string
        token = strtok(NULL, " \t\n\r");
    }
    
    // NULL-terminate the arguments array (required by execv and similar functions)
    args[*arg_count] = NULL;
    return args;
}

/**
 * Free memory allocated for command arguments
 * 
 * This function properly deallocates all memory associated with a command arguments array.
 * It first frees each individual argument string, then frees the array itself.
 * 
 * @param args: Array of string pointers to be freed
 * @param arg_count: Number of arguments in the array
 * 
 * Note: This function safely handles NULL pointers and should always be called
 * after using parse_command_args() to prevent memory leaks.
 */
void free_command_args(char **args, int arg_count) {
    if (args) {  // Check if args pointer is valid
        // Free each individual argument string
        for (int i = 0; i < arg_count; i++) {
            free(args[i]);  // Free the string allocated by strdup()
        }
        // Free the array of pointers itself
        free(args);
    }
}

/**
 * Setup input redirection for a command
 * 
 * @param input_file: Path to the input file
 * @return: File descriptor of the opened file, or -1 on error
 */
int setup_input_redirection(const char *input_file) {
    int fd = open(input_file, O_RDONLY);
    if (fd == -1) {
        printf("No such file or directory\n");
        return -1;
    }
    
    // Redirect stdin to the file
    if (dup2(fd, STDIN_FILENO) == -1) {
        perror("dup2");
        close(fd);
        return -1;
    }
    
    return fd;  // Return fd so caller can close it
}

/**
 * Set up output redirection for a command
 * 
 * This function handles output redirection by:
 * 1. Opening the specified output file with appropriate flags
 * 2. Redirecting stdout to the file using dup2()
 * 3. Returning the file descriptor for cleanup
 * 
 * @param output_file: Path to the output file
 * @param append_mode: 0 for overwrite (>), 1 for append (>>)
 * @return: File descriptor of the opened file, or -1 on error
 */
int setup_output_redirection(const char *output_file, int append_mode) {
    int flags;
    if (append_mode == 1) {
        // Append mode: create if doesn't exist, append if it does
        flags = O_CREAT | O_WRONLY | O_APPEND;
    } else {
        // Overwrite mode: create if doesn't exist, truncate if it does
        flags = O_CREAT | O_WRONLY | O_TRUNC;
    }
    
    // Open with read/write permissions for owner, read for group and others
    int fd = open(output_file, flags, 0644);
    if (fd == -1) {
        perror("open");
        return -1;
    }
    
    // Redirect stdout to the file
    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        close(fd);
        return -1;
    }
    
    return fd;  // Return fd so caller can close it
}

/**
 * Execute a command entered by the user
 * 
 * This is the main command dispatcher that:
 * 1. Parses the input command string into arguments
 * 2. Identifies the command type (built-in vs external)
 * 3. Calls the appropriate command handler
 * 4. Manages memory cleanup
 * 5. Adds successful commands to log
 * 
 * @param command: Raw command string entered by the user
 * 
 * Current supported built-in commands:
 * - hop: Directory navigation command
 * - reveal: Directory listing command
 * - log: Command history management
 * - exit: Terminate the shell
 */
void execute_command(char *command) {
    // Skip processing if command is empty or NULL
    // This handles cases where user just presses Enter
    if (command == NULL || strlen(command) == 0) {
        return;  // Nothing to execute
    }

    // Remove trailing newline for logging
    char *command_for_log = strdup(command);
    char *newline = strchr(command_for_log, '\n');
    if (newline) {
        *newline = '\0';
    }

    // Execute the command
    execute_command_without_logging(command);

    // Add to log after successful execution
    add_to_log(command_for_log);
    
    free(command_for_log);
}

/**
 * Execute a command without adding it to the log
 * Used by log execute functionality to avoid recursive logging
 */
/**
 * Execute a single command (no pipes) with full redirection support
 * 
 * This function handles execution of individual commands including:
 * - Built-in commands (hop, reveal, log, exit)
 * - External commands via execvp()
 * - Input redirection with < operator
 * - Output redirection with > and >> operators
 * - Proper process management and memory cleanup
 * 
 * The function creates a child process for most commands to ensure
 * consistent I/O redirection behavior, except for 'exit' which must
 * run in the parent process to actually terminate the shell.
 * 
 * @param command: Single command string with potential redirection operators
 */
void execute_single_command(char *command) {
    // Skip processing if command is empty or NULL
    if (command == NULL || strlen(command) == 0) {
        return;
    }
    
    // Create working copy for parsing
    char *command_copy = strdup(command);
    
    int arg_count;
    char *input_file;
    char *output_file;
    int append_mode;
    char **args = parse_command_args(command_copy, &arg_count, &input_file, &output_file, &append_mode);
    
    if (arg_count == 0) {
        free(command_copy);
        free_command_args(args, arg_count);
        if (input_file) free(input_file);
        if (output_file) free(output_file);
        return;
    }
    
    // Special case: exit command must run in parent to actually exit the shell
    if (strcmp(args[0], "exit") == 0) {
        printf("Goodbye!\n");
        exit(0);  // Terminate the program immediately
    }
    
    // All other commands (built-in and external) run in child process for consistent I/O handling
    pid_t pid = fork();  // Create a child process
    
    if (pid == 0) {
        // Child process: Set up redirection if needed, then execute command
        
        // Set up input redirection if specified
        if (input_file != NULL) {
            if (setup_input_redirection(input_file) == -1) {
                // Failed to open file or redirect - error already printed
                exit(1);
            }
        }
        
        // Set up output redirection if specified
        if (output_file != NULL && append_mode != -1) {
            if (setup_output_redirection(output_file, append_mode) == -1) {
                // Failed to open file or redirect - error already printed
                exit(1);
            }
        }
        
        // Command dispatch: Check the first argument to determine command type
        if (strcmp(args[0], "hop") == 0) {
            // Built-in hop command for directory navigation
            hop_command(args, arg_count);
            exit(0);  // Exit child process after built-in command
        } else if (strcmp(args[0], "reveal") == 0) {
            // Built-in reveal command for listing directory contents
            reveal_command(args, arg_count);
            exit(0);  // Exit child process after built-in command
        } else if (strcmp(args[0], "log") == 0) {
            // Built-in log command for command history
            log_command(args, arg_count);
            exit(0);  // Exit child process after built-in command
        } else if (strcmp(args[0], "activities") == 0) {
            // Built-in activities command for process tracking
            execute_activities_command(args, arg_count);
            exit(0);  // Exit child process after built-in command
        } else if (strcmp(args[0], "ping") == 0) {
            // Built-in ping command for sending signals to processes
            execute_ping_command(args, arg_count);
            exit(0);  // Exit child process after built-in command
        } else {
            // External command: Use execvp() to execute arbitrary programs
            // execvp() searches for the command in PATH and executes it
            if (execvp(args[0], args) == -1) {
                // execvp() failed - silently ignore and exit
                exit(1);  // Exit child process with error status (but no message)
            }
        }
    } else if (pid > 0) {
        // Parent process: Add to activities tracking and wait for child to complete
        add_activity(pid, command, 0);  // Track as foreground process
        
        // Set this as the current foreground process for signal handling
        set_foreground_process(pid);
        
        int status;
        waitpid(pid, &status, WUNTRACED);  // Block until child terminates OR stops
        
        // Check if process was stopped (Ctrl-Z) vs terminated
        if (WIFSTOPPED(status)) {
            // Process was stopped, don't remove from activities
            // Signal handler already moved it to background
        } else {
            // Process terminated normally, clean up
            remove_activity(pid);   // Remove from tracking when completed
        }
        
        // Clear foreground process tracking
        set_foreground_process(0);
    } else {
        // fork() failed - print error message
        perror("fork");  // Print system error message
    }
    
    // Memory cleanup: Always free allocated resources
    free(command_copy);              // Free the duplicated command string
    free_command_args(args, arg_count);  // Free the arguments array and its contents
    if (input_file) free(input_file);    // Free input file string if allocated
    if (output_file) free(output_file);  // Free output file string if allocated
}

void execute_command_without_logging(char *command) {
    // Skip processing if command is empty or NULL
    if (command == NULL || strlen(command) == 0) {
        return;
    }
    
    // Check if command contains background operator (&)
    if (contains_background_operator(command)) {
        // Remove the & operator and execute in background
        char *bg_command = strdup(command);
        if (bg_command) {
            remove_background_operator(bg_command);
            execute_background_command(bg_command);
            free(bg_command);
        }
        return;
    }
    
    // Check if command contains semicolons for sequential execution
    if (contains_semicolon(command)) {
        // Execute sequential commands using semicolon splitting approach
        execute_sequential_commands(command);
        return;
    }
    
    // Check if command contains pipes
    if (strchr(command, '|') != NULL) {
        // Execute pipeline using simple string splitting approach
        execute_pipeline(command);
        return;
    }
    
    // No special operators - use the dedicated single command function
    execute_single_command(command);
}

/**
 * Execute a pipeline of commands using simple string splitting approach
 * 
 * This function implements command piping by:
 * 1. Splitting the command string on pipe operators ('|')
 * 2. Creating pipes for inter-process communication
 * 3. Forking child processes for each command in the pipeline
 * 4. Setting up proper input/output redirection between processes
 * 5. Supporting both pipes and file redirection (< > >>) simultaneously
 * 
 * Pipeline Structure:
 * - First command: reads from stdin (or input file if < specified)
 * - Middle commands: read from previous pipe, write to next pipe
 * - Last command: writes to stdout (or output file if > or >> specified)
 * 
 * Process Management:
 * - Each command runs in its own child process
 * - Parent process waits for all children to complete
 * - Pipe file descriptors are properly closed to avoid deadlocks
 * 
 * Examples:
 * - "cat file.txt | grep pattern | wc -l" 
 * - "cat < input.txt | sort | uniq > output.txt"
 * - "reveal . | grep .txt >> results.txt"
 * 
 * @param command: Full command string containing pipe operators and/or redirection
 */
void execute_pipeline(char *command) {
    // Step 1: Create a working copy since we'll modify the string during parsing
    char *command_copy = strdup(command);
    
    // Step 2: Count pipe operators to determine the number of commands in pipeline
    // Each '|' separates two commands, so num_commands = pipe_count + 1
    int pipe_count = 0;
    char *temp = command_copy;
    while ((temp = strchr(temp, '|')) != NULL) {
        pipe_count++;
        temp++; // Move past the current pipe to find the next one
    }
    
    int num_commands = pipe_count + 1;
    
    // Step 3: Create pipes for inter-process communication
    // Note: Since we only reach this function when pipes are detected,
    // num_commands will always be >= 2
    // We need (num_commands - 1) pipes to connect num_commands processes
    // pipe_fds[i][0] = read end, pipe_fds[i][1] = write end
    int pipe_fds[num_commands - 1][2];
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipe_fds[i]) == -1) {
            perror("pipe");
            free(command_copy);
            return;
        }
    }
    
    // Step 4: Parse and execute each command in the pipeline
    char *cmd_start = command_copy;  // Pointer to start of current command
    pid_t pids[num_commands];        // Array to store child process IDs
    pid_t pipeline_pgid = 0;         // Process group ID for the entire pipeline
    
    for (int i = 0; i < num_commands; i++) {
        // Step 4a: Extract current command from the pipeline string
        // Find next pipe operator or end of string
        char *pipe_pos = strchr(cmd_start, '|');
        if (pipe_pos) {
            *pipe_pos = '\0';  // Null-terminate current command string
        }
        
        // Step 4b: Parse the individual command using existing parser
        // This handles redirection operators (<, >, >>) within each command
        int arg_count;
        char *input_file, *output_file;
        int append_mode;
        char **args = parse_command_args(cmd_start, &arg_count, &input_file, &output_file, &append_mode);
        
        // Skip empty commands (shouldn't happen with valid input)
        if (arg_count == 0) {
            if (pipe_pos) cmd_start = pipe_pos + 1;
            continue;
        }
        
        // Step 4c: Fork a child process for this command
        pids[i] = fork();
        if (pids[i] == 0) {
            // === CHILD PROCESS ===
            
            // Step 4c1: Set up process group
            if (i == 0) {
                // First process: create new process group with this process as leader
                pipeline_pgid = getpid();
                if (setpgid(0, 0) == -1) {
                    perror("setpgid");
                    exit(1);
                }
            } else {
                // Other processes: join the first process's group
                if (setpgid(0, pipeline_pgid) == -1) {
                    perror("setpgid");
                    exit(1);
                }
            }
            
            // Step 4d: Set up input redirection
            // Priority: pipe input > file input redirection
            if (i > 0) {
                // Not the first command: read from previous command's output pipe
                if (dup2(pipe_fds[i-1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
            } else if (input_file) {
                // First command with input file redirection (command < file)
                if (setup_input_redirection(input_file) == -1) exit(1);
            }
            
            // Step 4e: Set up output redirection  
            // Priority: pipe output > file output redirection
            if (i < num_commands - 1) {
                // Not the last command: write to next command's input pipe
                if (dup2(pipe_fds[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
            } else if (output_file && append_mode != -1) {
                // Last command with output file redirection (command > file or command >> file)
                if (setup_output_redirection(output_file, append_mode) == -1) exit(1);
            }
            
            // Step 4f: Close all pipe file descriptors in child process
            // This is crucial to prevent deadlocks and resource leaks
            // Child only needs the specific pipe ends that were dup2'd to stdin/stdout
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipe_fds[j][0]);  // Close read end
                close(pipe_fds[j][1]);  // Close write end
            }
            
            // Step 4g: Execute the command (built-in or external)
            if (strcmp(args[0], "hop") == 0) {
                hop_command(args, arg_count);
                exit(0);
            } else if (strcmp(args[0], "reveal") == 0) {
                reveal_command(args, arg_count);
                exit(0);
            } else if (strcmp(args[0], "log") == 0) {
                log_command(args, arg_count);
                exit(0);
            } else if (strcmp(args[0], "activities") == 0) {
                execute_activities_command(args, arg_count);
                exit(0);
            } else if (strcmp(args[0], "ping") == 0) {
                execute_ping_command(args, arg_count);
                exit(0);
            } else {
                // External command: use execvp to replace process image
                if (execvp(args[0], args) == -1) {
                    exit(1);  // Exit with error if exec fails
                }
            }
        } else if (pids[i] == -1) {
            // Fork failed
            perror("fork");
        } else {
            // === PARENT PROCESS ===
            
            // Set up process group in parent too (race condition prevention)
            if (i == 0) {
                // Store the process group ID from first process
                pipeline_pgid = pids[0];
                setpgid(pids[0], pids[0]);  // First process becomes group leader
            } else {
                // Add subsequent processes to the group
                setpgid(pids[i], pipeline_pgid);
            }
        }
        
        // Step 4h: Clean up resources for this command
        free_command_args(args, arg_count);
        if (input_file) free(input_file);
        if (output_file) free(output_file);
        
        // Step 4i: Move to next command in the pipeline
        if (pipe_pos) {
            cmd_start = pipe_pos + 1;
            // Skip whitespace after pipe operator for cleaner parsing
            while (*cmd_start == ' ' || *cmd_start == '\t') cmd_start++;
        }
    }
    
    // Step 5: Parent process cleanup and synchronization
    // Close all pipe file descriptors in parent to prevent deadlocks
    // Children have their own copies, so parent doesn't need them
    for (int i = 0; i < num_commands - 1; i++) {
        close(pipe_fds[i][0]);  // Close read end
        close(pipe_fds[i][1]);  // Close write end
    }
    
    // Step 6: Wait for all child processes to complete
    // Pipeline is only complete when ALL commands finish
    
    // For signal handling, track the entire pipeline as a process group
    if (pipeline_pgid > 0) {
        set_foreground_process_group(pipeline_pgid);
    }
    
    for (int i = 0; i < num_commands; i++) {
        if (pids[i] > 0) {
            int status;
            waitpid(pids[i], &status, WUNTRACED);  // Wait for child to terminate OR stop
            
            // If any process in pipeline is stopped, the whole pipeline is stopped
            // Signal handler will have already moved it to background
        }
    }
    
    // Clear foreground process group tracking
    set_foreground_process_group(0);
    
    // Step 7: Final cleanup
    free(command_copy);
}
