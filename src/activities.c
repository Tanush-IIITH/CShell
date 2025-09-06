#include "activities.h"

// Global activities tracking array
ActivityEntry activities[MAX_ACTIVITIES];
int activity_count = 0;

/* ===============================================
 * CORE FUNCTIONS
 * =============================================== */

/**
 * Initialize the activities tracking system
 */
void init_activities(void)
{
    // Initialize all activity slots as inactive
    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        activities[i].is_active = 0;
        activities[i].pid = 0;
        activities[i].command_name[0] = '\0';
        activities[i].full_command[0] = '\0';
        activities[i].state = PROC_TERMINATED;
        activities[i].is_background = 0;
    }
    activity_count = 0;
}

/**
 * Add a new process to activities tracking
 */
int add_activity(pid_t pid, const char *command, int is_background)
{
    // Find an available slot
    int slot = -1;
    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (!activities[i].is_active)
        {
            slot = i;
            break;
        }
    }

    if (slot == -1)
    {
        fprintf(stderr, "Error: Maximum number of activities reached\n");
        return -1;
    }

    // Fill in the activity entry
    activities[slot].pid = pid;
    activities[slot].is_active = 1;
    activities[slot].state = PROC_RUNNING; // New processes start as running
    activities[slot].is_background = is_background;

    // Store full command
    strncpy(activities[slot].full_command, command, sizeof(activities[slot].full_command) - 1);
    activities[slot].full_command[sizeof(activities[slot].full_command) - 1] = '\0';

    // Extract and store command name
    extract_command_name(command, activities[slot].command_name, sizeof(activities[slot].command_name));

    activity_count++;
    return 0;
}

/**
 * Remove a process from activities tracking
 */
int remove_activity(pid_t pid)
{
    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (activities[i].is_active && activities[i].pid == pid)
        {
            // Mark slot as inactive
            activities[i].is_active = 0;
            activities[i].pid = 0;
            activities[i].command_name[0] = '\0';
            activities[i].full_command[0] = '\0';
            activities[i].state = PROC_TERMINATED;
            activity_count--;
            return 0;
        }
    }
    return -1; // Not found
}

/**
 * Update the state of a process
 */
int update_activity_state(pid_t pid, ProcessState new_state)
{
    ActivityEntry *entry = find_activity(pid);
    if (entry)
    {
        entry->state = new_state;
        return 0;
    }
    return -1; // Not found
}

/**
 * Check all tracked processes and update their states
 */
void update_all_activities(void)
{
    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (activities[i].is_active)
        {
            ProcessState current_state = get_process_state(activities[i].pid);

            if (current_state == PROC_TERMINATED)
            {
                // Process has terminated, remove from tracking
                remove_activity(activities[i].pid);
            }
            else
            {
                // Update state if changed
                activities[i].state = current_state;
            }
        }
    }
}

/**
 * Comparison function for sorting activities by command name
 */
static int compare_activities(const void *a, const void *b)
{
    const ActivityEntry *entry_a = (const ActivityEntry *)a;
    const ActivityEntry *entry_b = (const ActivityEntry *)b;

    // Both inactive - order doesn't matter since they won't be displayed
    if (!entry_a->is_active && !entry_b->is_active)
        return 0;
    
    // One active, one inactive - active comes first
    if (!entry_a->is_active)
        return 1;  // Move inactive entry_a after active entry_b
    if (!entry_b->is_active)
        return -1; // Move inactive entry_b after active entry_a

    // Both active - sort by command name (lexicographic order)
    return strcmp(entry_a->command_name, entry_b->command_name);
}

/**
 * Display all active processes sorted by command name
 */
void display_activities(void)
{
    // First, update all activity states
    update_all_activities();

    // Create a copy of activities array for sorting
    ActivityEntry sorted_activities[MAX_ACTIVITIES];
    memcpy(sorted_activities, activities, sizeof(activities));

    // Sort by command name
    qsort(sorted_activities, MAX_ACTIVITIES, sizeof(ActivityEntry), compare_activities);

    // Display sorted active activities (optimized - early termination)
    int displayed = 0;
    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (!sorted_activities[i].is_active)
        {
            // Since inactive entries are sorted to the end, 
            // we can stop here - no more active entries
            break;
        }
        printf("[%d] : %s - %s\n",
               sorted_activities[i].pid,
               sorted_activities[i].command_name,
               state_to_string(sorted_activities[i].state));
        displayed++;
    }

    if (displayed == 0)
    {
        printf("No active processes\n");
    }
}

/**
 * Execute the activities command
 */
int execute_activities_command(char **args, int arg_count)
{
    // as the arguements are not being used here, the line below suppresses the unused parameter warning
    (void)args; // Suppress unused parameter warning

    if (arg_count != 1)
    {
        return -1;
    }

    display_activities();
    return 0;
}

/**
 * Extract command name from full command string
 */
void extract_command_name(const char *command, char *cmd_name, size_t max_len)
{
    if (!command || !cmd_name || max_len == 0)
    {
        if (cmd_name && max_len > 0)
            cmd_name[0] = '\0';
        return;
    }

    // Skip leading whitespace
    while (*command == ' ' || *command == '\t')
    {
        command++;
    }

    // Copy first word (command name)
    size_t i = 0;
    while (i < max_len - 1 && *command && *command != ' ' && *command != '\t')
    {
        cmd_name[i++] = *command++;
    }
    cmd_name[i] = '\0';
}

/**
 * Get the current state of a process
 */
ProcessState get_process_state(pid_t pid)
{
    char proc_path[256];
    char state_char;
    FILE *status_file;

    // Try to read process state from /proc/[pid]/stat
    // this is an abosolute path
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/stat", pid);
    status_file = fopen(proc_path, "r");

    if (!status_file)
    {
        // Process doesn't exist or we can't read it
        return PROC_TERMINATED;
    }

    // Parse the stat file - state is the 3rd field
    // Format: pid (comm) state ...
    if (fscanf(status_file, "%*d %*s %c", &state_char) == 1)
    {
        fclose(status_file);

        switch (state_char)
        {
        case 'R': // Running
        case 'S': // Sleeping (interruptible)
        case 'D': // Disk sleep (uninterruptible)
            return PROC_RUNNING;
        case 'T': // Stopped (on a signal)
        case 't': // Tracing stop
            return PROC_STOPPED;
        case 'Z': // Zombie
        case 'X': // Dead
        default:
            return PROC_TERMINATED;
        }
    }

    fclose(status_file);
    return PROC_TERMINATED;
}

/**
 * Convert ProcessState enum to string
 */
const char *state_to_string(ProcessState state)
{
    switch (state)
    {
    case PROC_RUNNING:
        return "Running";
    case PROC_STOPPED:
        return "Stopped";
    case PROC_TERMINATED:
        return "Terminated"; // Should not be displayed
    default:
        return "Unknown";
    }
}

/**
 * Find activity entry by PID
 */
ActivityEntry *find_activity(pid_t pid)
{
    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (activities[i].is_active && activities[i].pid == pid)
        {
            return &activities[i];
        }
    }
    return NULL;
}

/**
 * Clean up terminated processes from activities list
 */
void cleanup_terminated_activities(void)
{
    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (activities[i].is_active)
        {
            if (get_process_state(activities[i].pid) == PROC_TERMINATED)
            {
                remove_activity(activities[i].pid);
            }
        }
    }
}

/**
 * Kill all background processes and clean up activities list
 * 
 * This function is called during EOF (Ctrl-D) handling to terminate
 * all child processes before shell exit. It:
 * 1. Sends SIGKILL to all tracked processes
 * 2. Clears the activities list
 */
void cleanup_all_background_processes(void)
{
    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (activities[i].is_active)
        {
            // Send SIGKILL to terminate the process immediately
            kill(activities[i].pid, SIGKILL);
            
            // Mark as inactive
            activities[i].is_active = 0;
        }
    }
    
    // Reset activity count
    activity_count = 0;
}
