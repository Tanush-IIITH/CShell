#include "background.h"
#include "command.h"
#include "header.h"
#include "activities.h"
#include "signal_handler.h"

// Global array to track background jobs
static background_job_t background_jobs[MAX_BACKGROUND_JOBS];
static int next_job_number = 1;
static int job_count = 0;
static int most_recent_job = -1; // Track most recent job number

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
static char* extract_background_command_name(const char *command) {
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
    char *command_name = extract_background_command_name(command);
    
    // Fork the process
    pid_t pid = fork();
    
    if (pid == 0) {
        // === CHILD PROCESS ===
        
        // Create new process group for background process
        // This prevents it from receiving terminal signals (Ctrl-C, Ctrl-Z)
        if (setpgid(0, 0) == -1) {
            perror("setpgid");
            exit(1);
        }
        
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
        
        // Set the child to its own process group to prevent it from
        // receiving terminal signals (Ctrl-C, Ctrl-Z)
        // Do this in parent too to prevent race condition
        setpgid(pid, pid);
        
        // Register the background job
        background_jobs[slot].job_number = next_job_number;
        background_jobs[slot].pid = pid;
        background_jobs[slot].command_name = command_name;
        background_jobs[slot].is_active = 1;
        
        // Add to activities tracking
        add_activity(pid, command, 1);  // Track as background process
        
        // Print job information
        printf("[%d] %d\n", next_job_number, pid);
        
        // Update counters
        next_job_number++;
        job_count++;
        most_recent_job = background_jobs[slot].job_number; // Track most recent job
        
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
                
                // Remove from activities tracking
                remove_activity(pid);
                
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

/**
 * Add a stopped process to background jobs
 * 
 * Registers a process that was stopped via Ctrl-Z as a background job
 * 
 * @param pid: Process ID of the stopped process
 * @param command_name: Name of the command for display
 * @param is_stopped: 1 if process is stopped, 0 if running
 * @return: Job number assigned, or -1 on error
 */
int add_stopped_job(pid_t pid, const char *command_name, int is_stopped) {
    // Avoid unused parameter warning
    (void)is_stopped;
    
    // Find an available slot
    int slot = -1;
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (!background_jobs[i].is_active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        fprintf(stderr, "Maximum number of background jobs reached\n");
        return -1;
    }
    
    // Copy command name
    char *name_copy = strdup(command_name);
    if (!name_copy) {
        perror("strdup");
        return -1;
    }
    
    // Register the background job
    background_jobs[slot].job_number = next_job_number;
    background_jobs[slot].pid = pid;
    background_jobs[slot].command_name = name_copy;
    background_jobs[slot].is_active = 1;
    
    // Update counters
    int job_number = next_job_number;
    next_job_number++;
    job_count++;
    most_recent_job = job_number; // Track most recent job
    
    return job_number;
}

/**
 * Find job by job number
 */
static int find_job_by_number(int job_number) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].is_active && background_jobs[i].job_number == job_number) {
            return i;
        }
    }
    return -1;
}

/**
 * Bring a job to the foreground
 * 
 * @param job_number: Job number to bring to foreground (-1 for most recent)
 * @return: 0 on success, -1 on error
 */
int fg_command(int job_number) {
    int slot;
    
    // If job_number is -1, use most recent job
    if (job_number == -1) {
        if (most_recent_job == -1) {
            printf("No such job\n");
            return -1;
        }
        job_number = most_recent_job;
    }
    
    // Find the job
    slot = find_job_by_number(job_number);
    if (slot == -1) {
        printf("No such job\n");
        return -1;
    }
    
    pid_t pid = background_jobs[slot].pid;
    char *command_name = background_jobs[slot].command_name;
    
    // Print what we're bringing to foreground
    printf("%s\n", command_name);
    
    // Send SIGCONT to resume the process if it was stopped
    if (kill(pid, SIGCONT) == -1) {
        perror("fg: kill");
        return -1;
    }
    
    // Set this process as foreground for signal handling
    set_foreground_process(pid);
    
    // Remove from background jobs array
    free(background_jobs[slot].command_name);
    background_jobs[slot].is_active = 0;
    background_jobs[slot].command_name = NULL;
    job_count--;
    
    // Update most recent job if this was it
    if (job_number == most_recent_job) {
        most_recent_job = -1;
        // Find the next most recent job
        for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
            if (background_jobs[i].is_active && 
                (most_recent_job == -1 || background_jobs[i].job_number > most_recent_job)) {
                most_recent_job = background_jobs[i].job_number;
            }
        }
    }
    
    // Wait for the process to complete
    int status;
    if (waitpid(pid, &status, WUNTRACED) == -1) {
        perror("fg: waitpid");
        return -1;
    }
    
    // Check if process was stopped again
    if (WIFSTOPPED(status)) {
        // Process was stopped by signal handler which already added it back
    }
    
    // Clear foreground process
    set_foreground_process(0);
    
    return 0;
}

/**
 * Resume a job in the background
 * 
 * @param job_number: Job number to resume (-1 for most recent)
 * @return: 0 on success, -1 on error
 */
int bg_command(int job_number) {
    int slot;
    
    // If job_number is -1, use most recent job
    if (job_number == -1) {
        if (most_recent_job == -1) {
            printf("No such job\n");
            return -1;
        }
        job_number = most_recent_job;
    }
    
    // Find the job
    slot = find_job_by_number(job_number);
    if (slot == -1) {
        printf("No such job\n");
        return -1;
    }
    
    pid_t pid = background_jobs[slot].pid;
    char *command_name = background_jobs[slot].command_name;
    
    // Check if process is already running
    int status;
    if (waitpid(pid, &status, WNOHANG | WUNTRACED) == 0) {
        // Process is still running
        printf("Job already running\n");
        return 0;
    }
    
    // Send SIGCONT to resume the process
    if (kill(pid, SIGCONT) == -1) {
        perror("bg: kill");
        return -1;
    }
    
    // Print status
    printf("[%d]+ %s &\n", job_number, command_name);
    
    return 0;
}
