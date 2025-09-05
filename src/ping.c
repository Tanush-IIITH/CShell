#include "ping.h"
#include "activities.h"
#include <signal.h>
#include <errno.h>
#include <string.h>

int execute_ping_command(char **args, int arg_count) {
    // Validate argument count
    if (arg_count != 3) {
        fprintf(stderr, "Usage: ping <pid> <signal_number>\n");
        return -1;
    }
    
    // Parse PID
    char *endptr;
    long pid_long = strtol(args[1], &endptr, 10);
    if (*endptr != '\0' || pid_long <= 0) {
        fprintf(stderr, "Error: Invalid PID '%s'\n", args[1]);
        return -1;
    }
    pid_t pid = (pid_t)pid_long;
    
    // Parse signal number
    long signal_long = strtol(args[2], &endptr, 10);
    if (*endptr != '\0' || signal_long < 0) {
        fprintf(stderr, "Error: Invalid signal number '%s'\n", args[2]);
        return -1;
    }
    
    // Apply modulo 32 to signal number
    int signal_number = (int)(signal_long % 32);
    
    // Check if process exists in our tracking
    if (!is_tracked_process(pid)) {
        printf("No such process found\n");
        return -1;
    }
    
    // Send the signal
    if (send_signal_to_process(pid, signal_number) == 0) {
        printf("Sent signal %d to process with pid %d\n", signal_number, (int)pid);
        return 0;
    } else {
        return -1;
    }
}

int send_signal_to_process(pid_t pid, int signal_number) {
    // Use kill() to send the signal
    if (kill(pid, signal_number) == 0) {
        return 0; // Success
    } else {
        // Handle different error cases
        switch (errno) {
            case ESRCH:
                printf("No such process found\n");
                break;
            case EPERM:
                fprintf(stderr, "Error: Permission denied to send signal to process %d\n", (int)pid);
                break;
            case EINVAL:
                fprintf(stderr, "Error: Invalid signal number %d\n", signal_number);
                break;
            default:
                fprintf(stderr, "Error sending signal: %s\n", strerror(errno));
                break;
        }
        return -1;
    }
}

int is_tracked_process(pid_t pid) {
    // Use the existing find_activity function from activities module
    // This function returns a pointer to ActivityEntry if found, NULL if not found
    ActivityEntry* entry = find_activity(pid);
    return (entry != NULL) ? 1 : 0;
}
