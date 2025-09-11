#include "../include/header.h"
#include "../include/shell_input.h"
#include "../include/parser.h"
#include "../include/command.h"
#include "../include/hop.h"
#include "../include/log.h"
#include "../include/background.h"
#include "../include/activities.h"
#include "../include/signal_handler.h"

int main() {

    // Initialize the path for prompt display (~ substitution)
    init_shell_home();

    // Initialize signal handling (Ctrl-C protection)
    init_signal_handling();
    
    // Initialize hop command state
    init_hop_state();

    // Initialize background job management
    init_background_jobs();
    
    // Initialize activities tracking
    init_activities();
    
    while(1){
        // Check for completed background jobs before processing new input
        check_background_jobs();

        display_shell_prompt(); //display the shell prompt
        char command[4096];
        
        // Read command from user input
        if (!fgets(command, sizeof(command), stdin)) {
            // End of input reached (Ctrl-D pressed or pipe closed)
            // Handle EOF condition: kill all processes and exit cleanly
            handle_eof_condition();
            // Function never returns (calls exit(0))
        }

        // Skip empty commands
        if (strlen(command) <= 1) { // Only newline
            continue;
        }

        // Validate the command using the parser
        if (!validate_command(command)) {
            printf("Invalid Syntax!\n");
            continue;
        }

        // Execute the command
        execute_command(command);
    }
    
    //fix this later after implementing E3
    // Cleanup resources before exit (though this may never be reached)
    cleanup_background_jobs();
    return 0;
}