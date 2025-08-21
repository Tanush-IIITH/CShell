#include "../include/header.h"
#include "../include/parser.h"

/* ===============================================
 * LEXER IMPLEMENTATION
 * =============================================== */

// Create a new lexer with the given input string
Lexer* create_lexer(char *input) {
    Lexer *lexer = malloc(sizeof(Lexer));
    lexer->input = strdup(input);           // Create copy of input string
    lexer->position = 0;                    // Start at beginning
    lexer->length = strlen(input);          // Store total length
    lexer->current_token.value = NULL;      // No token initially
    return lexer;
}

// Free all memory allocated by the lexer
void free_lexer(Lexer *lexer) {
    if (lexer) {
        free(lexer->input);                 // Free input string copy
        if (lexer->current_token.value) {
            free(lexer->current_token.value); // Free current token's value
        }
        free(lexer);                        // Free lexer structure itself
    }
}

// Skip over whitespace characters (space, tab, newline, carriage return)
void skip_whitespace(Lexer *lexer) {
    while (lexer->position < lexer->length) {
        char c = lexer->input[lexer->position];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            lexer->position++;              // Move past whitespace
        } else {
            break;                          // Stop at first non-whitespace
        }
    }
}

// Get the next token from input - core tokenization function
Token next_token(Lexer *lexer) {
    Token token;
    token.value = NULL;
    token.position = lexer->position;       // Record where this token starts
    
    // Free previous token value to prevent memory leaks
    if (lexer->current_token.value) {
        free(lexer->current_token.value);
        lexer->current_token.value = NULL;
    }
    
    skip_whitespace(lexer);                 // Skip any leading whitespace
    
    // Check if we've reached end of input
    if (lexer->position >= lexer->length) {
        token.type = TOKEN_EOF;
        lexer->current_token = token;
        return token;
    }
    
    char c = lexer->input[lexer->position]; // Get current character
    
    // Tokenize based on current character
    switch (c) {
        case '|':
            token.type = TOKEN_PIPE;
            token.value = strdup("|");
            lexer->position++;              // Move past the '|'
            break;
            
        case ';':
            token.type = TOKEN_SEMICOLON;
            token.value = strdup(";");
            lexer->position++;              // Move past the ';'
            break;
            
        case '&':
            token.type = TOKEN_AMPERSAND;
            token.value = strdup("&");
            lexer->position++;              // Move past the '&'
            break;
            
        case '<':
            token.type = TOKEN_REDIRECT_IN;
            token.value = strdup("<");
            lexer->position++;              // Move past the '<'
            break;
            
        case '>':
            // Check for '>>' (append) vs '>' (overwrite)
            if (lexer->position + 1 < lexer->length && lexer->input[lexer->position + 1] == '>') {
                token.type = TOKEN_APPEND_OUT;
                token.value = strdup(">>");
                lexer->position += 2;       // Move past both '>' characters
            } else {
                token.type = TOKEN_REDIRECT_OUT;
                token.value = strdup(">");
                lexer->position++;          // Move past single '>'
            }
            break;
            
        default:
            // Parse name token (command, argument, filename)
            // Must not start with special characters
            if (c != '|' && c != '&' && c != '>' && c != '<' && c != ';') {
                int start = lexer->position;
                
                // Read characters until we hit a delimiter or whitespace
                while (lexer->position < lexer->length) {
                    c = lexer->input[lexer->position];
                    if (c == '|' || c == '&' || c == '>' || c == '<' || c == ';' || 
                        c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                        break;              // Stop at delimiter or whitespace
                    }
                    lexer->position++;
                }
                
                // Extract the token string if we found valid characters
                if (lexer->position > start) {
                    int length = lexer->position - start;
                    token.value = malloc(length + 1);
                    strncpy(token.value, lexer->input + start, length);
                    token.value[length] = '\0';  // Null terminate
                    token.type = TOKEN_NAME;
                } else {
                    token.type = TOKEN_ERROR;    // No valid characters found
                }
            } else {
                // Invalid character encountered
                token.type = TOKEN_ERROR;
                lexer->position++;              // Skip the invalid character
            }
            break;
    }
    
    lexer->current_token = token;              // Store as current token
    return token;
}

/* ===============================================
 * PARSER IMPLEMENTATION
 * =============================================== */

// Create parser with input string and get first token ready
Parser* create_parser(char *input) {
    Parser *parser = malloc(sizeof(Parser));
    parser->lexer = create_lexer(input);    // Create lexer for tokenization
    parser->error = 0;                      // Initialize with no errors
    next_token(parser->lexer);              // Get first token ready for parsing
    return parser;
}

// Free parser and its associated lexer
void free_parser(Parser *parser) {
    if (parser) {
        free_lexer(parser->lexer);          // Free the lexer first
        free(parser);                       // Then free parser structure
    }
}


/* ===============================================
 * GRAMMAR RULE PARSERS (Recursive Descent)
 * =============================================== */

// Parse input redirection: < name | <name
// Grammar rule: input -> < name | <name
int parse_input(Parser *parser) {
    if (parser->lexer->current_token.type == TOKEN_REDIRECT_IN) {
        next_token(parser->lexer);          // Consume '<'
        if (parser->lexer->current_token.type == TOKEN_NAME) {
            next_token(parser->lexer);      // Consume filename
            return 1;                       // Successfully parsed input redirection
        }
        parser->error = 1;                  // Missing filename after '<'
        return 0;
    }
    return 0;                               // Not an input redirection
}

// Parse output redirection: > name | >name | >> name | >>name
// Grammar rule: output -> > name | >name | >> name | >>name
int parse_output(Parser *parser) {
    if (parser->lexer->current_token.type == TOKEN_REDIRECT_OUT || 
        parser->lexer->current_token.type == TOKEN_APPEND_OUT) {
        next_token(parser->lexer);          // Consume '>' or '>>'
        if (parser->lexer->current_token.type == TOKEN_NAME) {
            next_token(parser->lexer);      // Consume filename
            return 1;                       // Successfully parsed output redirection
        }
        parser->error = 1;                  // Missing filename after '>' or '>>'
        return 0;
    }
    return 0;                               // Not an output redirection
}

// Parse atomic command: name (name | input | output)*
// Grammar rule: atomic -> name (name | input | output)*
// Examples: "ls", "cat file.txt", "grep pattern > output.txt"
int parse_atomic(Parser *parser) {
    // Must start with a command name
    if (parser->lexer->current_token.type != TOKEN_NAME) {
        parser->error = 1;                  // Error: expected command name
        return 0;
    }
    
    next_token(parser->lexer);              // Consume command name
    
    // Parse optional arguments and redirections
    // Loop continues while we see names (arguments) or redirections
    while (!parser->error && 
           (parser->lexer->current_token.type == TOKEN_NAME ||
            parser->lexer->current_token.type == TOKEN_REDIRECT_IN ||
            parser->lexer->current_token.type == TOKEN_REDIRECT_OUT ||
            parser->lexer->current_token.type == TOKEN_APPEND_OUT)) {
        
        if (parser->lexer->current_token.type == TOKEN_NAME) {
            next_token(parser->lexer);      // Consume argument
        } else if (!parse_input(parser) && !parse_output(parser)) {
            // Try to parse redirection; if both fail, it's an error
            parser->error = 1;
            return 0;
        }
    }
    
    return !parser->error;                  // Success if no errors occurred
}

// Parse command group (pipeline): atomic (| atomic)*
// Grammar rule: cmd_group -> atomic (| atomic)*
// Examples: "ls", "cat file.txt | grep pattern", "ls | sort | uniq"
int parse_cmd_group(Parser *parser) {
    // Must start with at least one atomic command
    if (!parse_atomic(parser)) {
        return 0;                           // Failed to parse first command
    }
    
    // Parse optional pipe-connected commands
    while (!parser->error && parser->lexer->current_token.type == TOKEN_PIPE) {
        next_token(parser->lexer);          // Consume '|'
        if (!parse_atomic(parser)) {
            parser->error = 1;              // Error: expected command after pipe
            return 0;
        }
    }
    
    return !parser->error;                  // Success if no errors
}

// Parse complete shell command: cmd_group ((& | ;) cmd_group)* &?
// Grammar rule: shell_cmd -> cmd_group ((& | ;) cmd_group)* &?
// Examples: "ls", "ls ; pwd", "cat file.txt | grep pattern &", "ls ; pwd ; date &"
int parse_shell_cmd(Parser *parser) {
    // Must start with at least one command group
    if (!parse_cmd_group(parser)) {
        return 0;                           // Failed to parse first command group
    }
    
    // Parse optional sequences connected by ';' or '&'
    while (!parser->error && 
           (parser->lexer->current_token.type == TOKEN_AMPERSAND ||
            parser->lexer->current_token.type == TOKEN_SEMICOLON)) {
        
        TokenType op = parser->lexer->current_token.type; //save the operator type to check for background execution
        next_token(parser->lexer);          // Consume ';' or '&'
        
        // Special case: '&' at end of command is OK (runs entire command in background)
        if (op == TOKEN_AMPERSAND && parser->lexer->current_token.type == TOKEN_EOF) {
            break;                          // Found trailing '&', stop parsing
        }
        
        // Otherwise, we need another command group after the operator
        if (!parse_cmd_group(parser)) {
            parser->error = 1;              // Error: expected command after ';' or '&'
            return 0;
        }
    }
    
    // Handle optional final '&' (background execution)
    if (parser->lexer->current_token.type == TOKEN_AMPERSAND) {
        next_token(parser->lexer);          // Consume final '&'
    }
    
    return !parser->error;                  // Success if no errors
}

/* ===============================================
 * PUBLIC API
 * =============================================== */

// Main validation function - validates a complete shell command
// Returns 1 if command is syntactically valid, 0 if invalid
// This is the only function other modules should call
int validate_command(char *command) {
    // Remove trailing newline from fgets 
    char *newline = strchr(command, '\n');
    if (newline) {
        *newline = '\0';                    // Replace newline with null terminator
    }
    
    // Empty command is considered valid (user just pressed Enter)
    if (strlen(command) == 0) {
        return 1;
    }
    
    // Create parser and attempt to parse the command
    Parser *parser = create_parser(command);
    int valid = parse_shell_cmd(parser);    // Parse according to our grammar
    
    // Ensure we consumed all input - leftover tokens indicate syntax error
    // TOKEN_EOF is kind of redundant here, but we check it for completeness
    if (valid && parser->lexer->current_token.type != TOKEN_EOF) {
        valid = 0;                          // Invalid: didn't consume entire input
    }
    
    free_parser(parser);                    // Clean up parser memory
    return valid;                           // Return validation result
}
