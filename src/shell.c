#include "../include/header.h"
#include "../include/shell_input.h"
#include "../include/parser.h"
#include "../include/command.h"
#include "../include/hop.h"
#include "../include/log.h"

int main() {
    // Initialize hop command state
    init_hop_state();
    
    // Initialize log system
    init_log();
    
    while(1){
        display_shell_prompt(); //display the shell prompt
        char command[4096];
        fgets(command, sizeof(command), stdin); //read command from user input

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
    
    return 0;
}