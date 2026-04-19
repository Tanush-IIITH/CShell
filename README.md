# Custom POSIX-Compliant C-Shell

A robust, modular, and POSIX-compliant Unix shell implementation in C. This project mimics core functionalities of major shells like Bash or Zsh, featuring a custom recursive descent parser, advanced process management, and a suite of built-in commands.

## Overview

This C-Shell is built from scratch to demonstrate a deep understanding of Operating System concepts, specifically process creation, signal handling, and inter-process communication (IPC). It parses complex user inputs defined by a custom Context-Free Grammar (CFG) and executes them using low-level system calls.

## Key Features

### 1. Advanced Command Execution
* **Foreground & Background Processes:** Seamlessly manages foreground tasks and background jobs (using `&`) with a robust job control system.
* **Sequential Execution:** Supports chaining multiple commands using the semicolon `;` operator.
* **Process Management:** Custom implementation of process spawning using `fork()`, `execvp()`, and `waitpid()`.

### 2. I/O Redirection & Pipelines
* **Piping:** Enables multi-stage command pipelines (`cmd1 | cmd2 | cmd3`) using `pipe()` and `dup2()` to connect `stdout` of one process to `stdin` of the next.
* **Redirection:** Full support for input redirection (`<`), output overwriting (`>`), and output appending (`>>`).

### 3. Custom Parser
* **Recursive Descent Parser:** Implements a custom lexer and parser to tokenize and validate input against a specific Context-Free Grammar (CFG).
* **Robust Tokenization:** Handles complex combinations of pipes, redirections, and background operators.

### 4. Job Control & Signals
* **Signal Handling:** Custom handlers for `SIGINT` (Ctrl-C) and `SIGTSTP` (Ctrl-Z) to manage process groups without terminating the shell.
* **Job Commands:**
    * `activities`: Lists all active processes spawned by the shell with their current state (Running/Stopped).
    * `fg [job_id]`: Brings a background process to the foreground.
    * `bg [job_id]`: Resumes a stopped process in the background.
    * `ping <pid> <signal>`: Sends specific signals to processes (modulo 32).

### 5. Built-in Intrinsics
* **`hop`:** Navigates directories with support for flags like `.` (current), `..` (parent), `~` (home), and `-` (previous directory).
* **`reveal`:** Lists directory contents with support for hidden files (`-a`) and long-listing format (`-l`), sorted lexicographically.
* **`log`:** Maintains a persistent history of the last 15 commands across sessions, with capabilities to `purge` or `execute` commands by index.

## Architecture

The project is structured into multiple modules to ensure separation of concerns:

| Module | Description |
| :--- | :--- |
| **`shell.c`** | Main entry point; handles the REPL loop and prompt display. |
| **`parser.c`** | Lexical analysis and parsing of user input. |
| **`command.c`** | Handles command dispatch, redirection setup, and pipelining logic. |
| **`background.c`** | Manages background job tracking and status reporting. |
| **`sequential.c`** | Handles sequential command execution logic. |
| **`signal_handler.c`** | Contains handlers for interrupts and job control signals. |
| **`activities.c`** | Manages process tracking and state updates. |
| **`builtins`** | Implementations for `hop`, `reveal`, `log`, etc. |

## Installation & Usage

### Prerequisites
* GCC Compiler
* Make
* Linux/Unix Environment (or WSL)

### Build
The project uses a `Makefile` for easy compilation.
```bash
# Clone the repository
git clone [https://github.com/yourusername/c-shell.git](https://github.com/yourusername/c-shell.git)

# Navigate to the shell directory
cd c-shell/shell

# Compile the shell
make

# Run
./shell.out

# Cleanup
make clean