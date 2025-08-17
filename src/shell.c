#include "../include/header.h"
#include "../include/shell_input.h"

int main() {
    while(1){
        display_shell_prompt(); //display the shell prompt
        char command[1024];
        fgets(command, sizeof(command), stdin); //read command from user input
        
    }
    
    return 0;
}