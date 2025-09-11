#include "../include/header.h"
#include "../include/shell_input.h"

static char shell_home_directory[1024] = "";

// Function to initialize the shell's home directory (the startup path)
void init_shell_home(void) {
    if (getcwd(shell_home_directory, sizeof(shell_home_directory)) == NULL) {
        // Fallback to root if getcwd fails
        strcpy(shell_home_directory, "/");
    }
}

// Get username
char* get_username(void) {
    struct passwd *pw = getpwuid(getuid());
    return pw ? pw->pw_name : "unknown";
}

// Get hostname 
char* get_hostname(void) {
    static char hostname[256];
    
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return hostname;
    }
    return "localhost";
}

// Get current working directory with ~ substitution for home 
char* get_current_path(void) {
    static char display_path[1024];
    
    char *pwd = getcwd(NULL, 0);
    if (pwd == NULL) {
        strcpy(display_path, "/");
        return display_path;
    }
    
    // Get home directly from passwd database
    char *home = shell_home_directory;
    
    if (home != NULL) {
        size_t home_len = strlen(home);
        // Simple check: if pwd starts with home path
        if (strncmp(pwd, home, home_len) == 0 && //checks if pwd starts from home
            (pwd[home_len] == '\0' || pwd[home_len] == '/')) { //exactly home or subdirectory
            
            if (pwd[home_len] == '\0') {
                // Exactly home directory
                strcpy(display_path, "~");
            } else {
                // Subdirectory of home
                snprintf(display_path, sizeof(display_path), "~%s", pwd + home_len);
            }
        } else {
            // Outside home, use full path
            strncpy(display_path, pwd, sizeof(display_path) - 1);
            display_path[sizeof(display_path) - 1] = '\0';
        }
    } else {
        // No HOME set, use full path
        strncpy(display_path, pwd, sizeof(display_path) - 1);
        display_path[sizeof(display_path) - 1] = '\0';
    }
    return display_path;
}

// Display the shell prompt in format: <username@hostname:current_path>
void display_shell_prompt(void) {
    char *username = get_username();
    char *hostname = get_hostname();
    char *current_path = get_current_path();
    
    printf("<%s@%s:%s> ", username, hostname, current_path);
    fflush(stdout);  // Ensure prompt is displayed immediately
}

