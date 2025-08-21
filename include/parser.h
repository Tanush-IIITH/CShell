#pragma once
#include "header.h"

// Token types for the lexer - represents different kinds of symbols in shell commands
typedef enum {
    TOKEN_NAME,         // Command names, arguments, filenames (e.g., "ls", "-la", "file.txt")
    TOKEN_PIPE,         // | (pipe operator - connects commands in pipeline)
    TOKEN_SEMICOLON,    // ; (sequential execution - run commands one after another)
    TOKEN_AMPERSAND,    // & (background execution - run command in background)
    TOKEN_REDIRECT_IN,  // < (input redirection - read from file)
    TOKEN_REDIRECT_OUT, // > (output redirection - write to file, overwrite)
    TOKEN_APPEND_OUT,   // >> (append redirection - write to file, append)
    TOKEN_EOF,          // End of input - no more tokens to read
    TOKEN_ERROR         // Invalid token - something that doesn't match any rule
} TokenType;

// Token structure - represents a single meaningful unit from the input
typedef struct {
    TokenType type;     // What kind of token this is 
    char *value;        // The actual text of the token (e.g., "ls", "|", "file.txt")
    int position;       // Position in input string where this token was found
} Token;

// Lexer structure - manages the process of breaking input into tokens
typedef struct {
    char *input;        // Input string to be tokenized (copy of user's command)
    int position;       // Current position in the input string being processed
    int length;         // Total length of input string
    Token current_token; // The token we're currently looking at/processing
} Lexer;

// Parser structure - manages the process of checking if tokens follow grammar rules
typedef struct {
    Lexer *lexer;       // Pointer to lexer that provides tokens
    int error;          // Error flag: 0 = no error, 1 = syntax error found
} Parser;

// Lexer lifecycle management
Lexer* create_lexer(char *input);    // Create new lexer with input string
void free_lexer(Lexer *lexer);       // Clean up lexer memory

// Token processing functions
Token next_token(Lexer *lexer);      // Get next token from input and advance position
void skip_whitespace(Lexer *lexer);  // Skip spaces, tabs, newlines, carriage returns

// Parser lifecycle management
Parser* create_parser(char *input);  // Create new parser with input string
void free_parser(Parser *parser);    // Clean up parser and lexer memory

// Grammar rule parsing functions (recursive descent parser)
// Each function corresponds to a rule in our context-free grammar:

int parse_shell_cmd(Parser *parser);   // shell_cmd -> cmd_group ((& | ;) cmd_group)* &?
int parse_cmd_group(Parser *parser);   // cmd_group -> atomic (| atomic)*
int parse_atomic(Parser *parser);      // atomic -> name (name | input | output)*
int parse_input(Parser *parser);       // input -> < name | <name
int parse_output(Parser *parser);      // output -> > name | >name | >> name | >>name

// Main validation function - the only function other modules need to call
int validate_command(char *command);   // Returns 1 if command is valid, 0 if invalid
