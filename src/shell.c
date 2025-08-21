#include "../include/header.h"
#include "../include/shell_input.h"
#include "../include/parser.h"

int main() {
    while(1){
        display_shell_prompt(); //display the shell prompt
        char command[4096];
        fgets(command, sizeof(command), stdin); //read command from user input

        // Validate the command using the parser
        if (!validate_command(command)) {
            printf("Invalid Syntax!\n");
        }

    }
    
    return 0;
}