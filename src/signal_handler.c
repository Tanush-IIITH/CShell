#include "signal_handler.h"
#include "shell_input.h"
#include "activities.h"
#include "background.h"
#include <signal.h>
#include <sys/types.h>

// Global variables to track current foreground process or process group
static pid_t current_foreground_pid = 0;    // For single commands
static pid_t current_foreground_pgid = 0;   // For pipelines (process groups)

/**
 * SIGINT signal handler
 * 
 * This function is called when Ctrl-C (SIGINT) is pressed.
 * It forwards the SIGINT signal to the current foreground process or process group
 * if one exists, otherwise it does nothing (preventing shell termination).
 * 
 * @param sig: The signal number (should be SIGINT = 2)
 */
static void sigint_handler(int sig) {
    // Avoid unused parameter warning
    (void)sig;
    
    // Priority: Process group (pipeline) > Single process
    if (current_foreground_pgid > 0) {
        printf("\n"); // Move to new line after ^C
        
        // Send SIGINT to entire process group (negative PID signals the group)
        if (kill(-current_foreground_pgid, SIGINT) == 0) {
            // Signal sent successfully to entire pipeline
        } else {
            // Process group might have already terminated
            current_foreground_pgid = 0;
        }
    } else if (current_foreground_pid > 0) {
        printf("\n"); // Move to new line after ^C
        
        // Use kill() to send SIGINT to the single foreground process
        if (kill(current_foreground_pid, SIGINT) == 0) {
            // Signal sent successfully
        } else {
            // Process might have already terminated
            current_foreground_pid = 0;
        }
    } else {
        // No foreground process, just print a newline and redisplay prompt
        printf("\n");
        display_shell_prompt();  // Redisplay the shell prompt
        fflush(stdout);
    }
}

/**
 * SIGTSTP signal handler
 * 
 * This function is called when Ctrl-Z (SIGTSTP) is pressed.
 * It forwards the SIGTSTP signal to the current foreground process or process group
 * if one exists, moves the stopped process to background, and prints job info.
 * The shell itself does not stop.
 * 
 * @param sig: The signal number (should be SIGTSTP = 20)
 */
static void sigtstp_handler(int sig) {
    // Avoid unused parameter warning
    (void)sig;
    
    // Priority: Process group (pipeline) > Single process
    if (current_foreground_pgid > 0) {
        printf("\n"); // Move to new line after ^Z
        
        // Send SIGTSTP to entire process group (negative PID signals the group)
        if (kill(-current_foreground_pgid, SIGTSTP) == 0) {
            // Signal sent successfully to entire pipeline
            // Move the process group to background with "Stopped" status
            move_process_group_to_background(current_foreground_pgid);
        } else {
            // Process group might have already terminated
            current_foreground_pgid = 0;
        }
    } else if (current_foreground_pid > 0) {
        printf("\n"); // Move to new line after ^Z
        
        // Send SIGTSTP to the single foreground process
        if (kill(current_foreground_pid, SIGTSTP) == 0) {
            // Signal sent successfully
            // Move the process to background with "Stopped" status
            move_process_to_background(current_foreground_pid);
        } else {
            // Process might have already terminated
            current_foreground_pid = 0;
        }
    } else {
        // No foreground process, just print a newline and redisplay prompt
        printf("\n");
        display_shell_prompt();  // Redisplay the shell prompt
        fflush(stdout);
    }
}

/**
 * Initialize signal handling for the shell
 * 
 * Sets up the SIGINT handler to prevent the shell from terminating
 * when Ctrl-C is pressed, and instead forward the signal to foreground processes.
 */
void init_signal_handling(void) {
    // Install our custom SIGINT handler
    struct sigaction sa_int;
    sa_int.sa_handler = sigint_handler;    // Our custom handler function
    sigemptyset(&sa_int.sa_mask);          // Don't block other signals during handler
    sa_int.sa_flags = SA_RESTART;          // Restart interrupted system calls
    
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        // Continue anyway - shell will still work without proper signal handling
    }
    
    // Install our custom SIGTSTP handler
    struct sigaction sa_tstp;
    sa_tstp.sa_handler = sigtstp_handler;  // Our custom handler function
    sigemptyset(&sa_tstp.sa_mask);         // Don't block other signals during handler
    sa_tstp.sa_flags = SA_RESTART;         // Restart interrupted system calls
    
    if (sigaction(SIGTSTP, &sa_tstp, NULL) == -1) {
        perror("sigaction SIGTSTP");
        // Continue anyway - shell will still work without proper signal handling
    }
    
    // Note: We don't handle SIGQUIT (Ctrl-\) or other signals here
    // The shell should still be terminable via SIGQUIT if needed
}

/**
 * Set the current foreground process PID (for single commands)
 * 
 * This should be called:
 * - When starting a foreground process (set to the process PID)
 * - When a foreground process completes (set to 0)
 * 
 * @param pid: Process ID of the foreground process (0 if no foreground process)
 */
void set_foreground_process(pid_t pid) {
    current_foreground_pid = pid;
    // Clear process group tracking when setting single process
    if (pid > 0) {
        current_foreground_pgid = 0;
    }
}

/**
 * Set the current foreground process group (for pipelines)
 * 
 * This should be called:
 * - When starting a pipeline (set to the process group ID)
 * - When a pipeline completes (set to 0)
 * 
 * @param pgid: Process Group ID of the foreground pipeline (0 if no foreground group)
 */
void set_foreground_process_group(pid_t pgid) {
    current_foreground_pgid = pgid;
    // Clear single process tracking when setting process group
    if (pgid > 0) {
        current_foreground_pid = 0;
    }
}

/**
 * Get the current foreground process PID
 * 
 * @return: PID of current foreground process, or 0 if none
 */
pid_t get_foreground_process(void) {
    return current_foreground_pid;
}

/**
 * Handle EOF (Ctrl-D) condition
 * 
 * This function is called when EOF is detected (user presses Ctrl-D).
 * It performs cleanup by:
 * 1. Killing all active child processes (foreground and background)
 * 2. Printing "logout" message
 * 3. Exiting the shell with status 0
 */
void handle_eof_condition(void) {
    printf("logout\n");
    
    // Kill current foreground process if any
    if (current_foreground_pid > 0) {
        kill(current_foreground_pid, SIGKILL);
    }
    
    // Kill current foreground process group if any
    if (current_foreground_pgid > 0) {
        kill(-current_foreground_pgid, SIGKILL);
    }
    
    // Kill all background processes tracked by activities
    // This requires integration with activities system
    cleanup_all_background_processes();
    
    // Exit shell with success status
    exit(0);
}

/**
 * Move a single process to background with "Stopped" status
 * 
 * This function is called when Ctrl-Z is pressed on a single command.
 * It adds the process to the background job list and prints job information.
 * 
 * @param pid: Process ID to move to background
 */
void move_process_to_background(pid_t pid) {
    // Find the command name from activities
    ActivityEntry* activity = find_activity(pid);
    char* command_name = "unknown";
    
    if (activity != NULL) {
        command_name = activity->command_name;
        // Update activity status to stopped
        activity->state = PROC_STOPPED;
        activity->is_background = 1;
    }
    
    // Add to background jobs list
    int job_number = add_stopped_job(pid, command_name, 1); // 1 = stopped
    
    // Print job notification: [job_number] Stopped command_name
    printf("[%d] Stopped %s\n", job_number, command_name);
    
    // Clear foreground tracking
    current_foreground_pid = 0;
    
    // Don't display prompt here - let main loop handle it
    fflush(stdout);
}

/**
 * Move a process group to background with "Stopped" status
 * 
 * This function is called when Ctrl-Z is pressed on a pipeline.
 * Since the group leader represents the entire pipeline, we can
 * simply call move_process_to_background() with the group leader PID.
 * 
 * @param pgid: Process Group ID to move to background
 */
void move_process_group_to_background(pid_t pgid) {
    // Update all processes in the group to stopped status in activities
    for (int i = 0; i < MAX_ACTIVITIES; i++) {
        if (activities[i].is_active && getpgid(activities[i].pid) == pgid) {
            activities[i].state = PROC_STOPPED;
            activities[i].is_background = 1;
        }
    }
    
    // The group leader (pgid) represents the entire pipeline
    // Use the single process function to handle job management
    move_process_to_background(pgid);
    
    // Clear foreground group tracking (single process function clears PID tracking)
    current_foreground_pgid = 0;
}
