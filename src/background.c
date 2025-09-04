#include "background.h"
#include "command.h"
#include "header.h"

// Global array to track background jobs
static background_job_t background_jobs[MAX_BACKGROUND_JOBS];
static int next_job_number = 1;
static int job_count = 0;

/**
 * Initialize the background job management system
 */
void init_background_jobs(void) {
    // Initialize all job slots as inactive
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        background_jobs[i].is_active = 0;
        background_jobs[i].command_name = NULL;
    }
    
    next_job_number = 1;
    job_count = 0;
    
}

/**
 * Check if a command string contains the background operator
 */
int contains_background_operator(const char *command) {
    if (command == NULL || strlen(command) == 0) {
        return 0;
    }
    
    // Find the last non-whitespace character
    int len = strlen(command);
    int i = len - 1;
    
    // Skip trailing whitespace
    while (i >= 0 && (command[i] == ' ' || command[i] == '\t' || command[i] == '\n')) {
        i--;
    }
    
    // Check if the last non-whitespace character is &
    return (i >= 0 && command[i] == '&');
}

/**
 * Remove the background operator from a command string
 */
void remove_background_operator(char *command) {
    if (command == NULL) {
        return;
    }
    
    int len = strlen(command);
    int i = len - 1;
    
    // Find the & character (working backwards)
    while (i >= 0 && (command[i] == ' ' || command[i] == '\t' || command[i] == '\n')) {
        i--;
    }
    
    // Remove the & and any trailing whitespace
    if (i >= 0 && command[i] == '&') {
        command[i] = '\0';
        
        // Remove any trailing whitespace before the &
        i--;
        while (i >= 0 && (command[i] == ' ' || command[i] == '\t')) {
            i--;
        }
        command[i + 1] = '\0';
    }
}

/**
 * Extract command name from command string for reporting
 */
static char* extract_command_name(const char *command) {
    if (command == NULL) {
        return strdup("unknown");
    }
    
    // Skip leading whitespace
    while (*command == ' ' || *command == '\t') {
        command++;
    }
    
    // Find the end of the first word (command name)
    const char *end = command;
    while (*end != '\0' && *end != ' ' && *end != '\t') {
        end++;
    }
    
    // Extract the command name
    int name_len = end - command;
    char *name = malloc(name_len + 1);
    if (name) {
        strncpy(name, command, name_len);
        name[name_len] = '\0';
    }
    
    return name ? name : strdup("unknown");
}

/**
 * Find an available job slot
 */
static int find_available_job_slot(void) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (!background_jobs[i].is_active) {
            return i;
        }
    }
    return -1; // No available slots
}

/**
 * Execute a command in the background
 */
int execute_background_command(char *command) {
    // Find available job slot
    int slot = find_available_job_slot();
    if (slot == -1) {
        fprintf(stderr, "Error: Maximum number of background jobs reached\n");
        return -1;
    }
    
    // Extract command name for reporting
    char *command_name = extract_command_name(command);
    
    // Fork the process
    pid_t pid = fork();
    
    if (pid == 0) {
        // === CHILD PROCESS ===
        
        // Redirect stdin to /dev/null to prevent background processes
        // from accessing terminal input
        int null_fd = open("/dev/null", O_RDONLY);
        if (null_fd != -1) {
            dup2(null_fd, STDIN_FILENO);
            close(null_fd);
        }
        
        // Remove the background operator from the command before execution
        // to prevent infinite recursion
        char *clean_command = strdup(command);
        remove_background_operator(clean_command);
        
        // Execute the command using the existing command execution infrastructure
        execute_single_command(clean_command);
        
        free(clean_command);
        exit(0); // Child should exit after execution
        
    } else if (pid > 0) {
        // === PARENT PROCESS ===
        
        // Register the background job
        background_jobs[slot].job_number = next_job_number;
        background_jobs[slot].pid = pid;
        background_jobs[slot].command_name = command_name;
        background_jobs[slot].is_active = 1;
        
        // Print job information
        printf("[%d] %d\n", next_job_number, pid);
        
        // Update counters
        next_job_number++;
        job_count++;
        
        return 0;
        
    } else {
        // Fork failed
        perror("fork");
        free(command_name);
        return -1;
    }
}

/**
 * Check for completed background jobs
 */
void check_background_jobs(void) {
    int status;
    pid_t pid;
    
    // Check for any completed child processes (non-blocking)
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Find the job with this PID
        for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
            if (background_jobs[i].is_active && background_jobs[i].pid == pid) {
                // Report job completion
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    printf("%s with pid %d exited normally\n", 
                           background_jobs[i].command_name, pid);
                } else {
                    printf("%s with pid %d exited abnormally\n", 
                           background_jobs[i].command_name, pid);
                }
                
                // Clean up job slot
                free(background_jobs[i].command_name);
                background_jobs[i].is_active = 0;
                background_jobs[i].command_name = NULL;
                job_count--;
                break;
            }
        }
    }
}
/**
 * Clean up background job tracking
 */
void cleanup_background_jobs(void) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].command_name) {
            free(background_jobs[i].command_name);
            background_jobs[i].command_name = NULL;
        }
        background_jobs[i].is_active = 0;
    }
    job_count = 0;
}
