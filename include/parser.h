#pragma once
#include "header.h"

// Token types for the lexer
typedef enum {
    TOKEN_NAME,         // Command names, arguments, filenames
    TOKEN_PIPE,         // |
    TOKEN_SEMICOLON,    // ;
    TOKEN_AMPERSAND,    // &
    TOKEN_REDIRECT_IN,  // <
    TOKEN_REDIRECT_OUT, // >
    TOKEN_APPEND_OUT,   // >>
    TOKEN_EOF,          // End of input
    TOKEN_ERROR         // Invalid token
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char *value;        // The actual text of the token
    int position;       // Position in input string
} Token;

// Lexer structure
typedef struct {
    char *input;        // Input string
    int position;       // Current position
    int length;         // Length of input
    Token current_token; // Current token
} Lexer;

// Parser structure
typedef struct {
    Lexer *lexer;
    int error;          // Error flag
} Parser;

// Function declarations
Lexer* create_lexer(char *input);
void free_lexer(Lexer *lexer);
Token next_token(Lexer *lexer);
void skip_whitespace(Lexer *lexer);

Parser* create_parser(char *input);
void free_parser(Parser *parser);
int parse_shell_cmd(Parser *parser);
int parse_cmd_group(Parser *parser);
int parse_atomic(Parser *parser);
int parse_input(Parser *parser);
int parse_output(Parser *parser);

// Main validation function
int validate_command(char *command);
