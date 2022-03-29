#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// -*--*--*--*--*-- COLORED COMMAND LINE --*--*--*--*--*--*--*-
#define K_NRM  "\x1B[0m"
#define K_RED  "\x1B[31m"
#define K_GRN  "\x1B[32m"
#define K_YEL  "\x1B[33m"
#define K_BLU  "\x1B[34m"
#define K_MAG  "\x1B[35m"
#define K_CYN  "\x1B[36m"
#define K_WHT  "\x1B[37m"
// -*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*-

#define MAX_CHAR_COUNT 100
#define MAX_ARG_COUNT 10
#define HISTORY_LIMIT 10

#define READ_END 0
#define WRITE_END 1

// -*--*--*--*--*--*--*-- HISTORY *--*--*--*--*--*--*--*--*--*-
char history[HISTORY_LIMIT][MAX_CHAR_COUNT] = {0};

// Add to history
void enqueueHistory(char *command);

// print history
void printHistoryQueue();
// -*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*-

// -*--*--*--*- SIMPLE SHELL & BUILT-IN FUNCTIONS --*--*--*--*-
void startShell();

void closeShell();

void printHelp();

// prints command prefix
void printShell(char *name);

// prints red colored failuRe message
void printFailure(char *message);

// change working directory (built in)
void changeDirectory(char **parsed);

// prints the current working directory (built in)
void printDirectory();

// takes the command entered, if succeed command assigned to input, returned 0
int getInput(char *input);
// -*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*-

// -*--*--*--*- PREPARE THE ENTERED COMMAND --*--*--*--*--*--*-

// splits the command with "|"
// assigns the first command (char*) to parsedPipe [0]
// the second command (char*) to parsedPipe [1]
// returns 1 if str contains pipe
int parseByPipe(char *str, char **parsedPipe);

// get tokens from the command seperated with " "
// assigns the tokens to parsed entries, until it founds NULL token
void parseBySpace(char *str, char **parsed);

// processes String
// first parseByPipe the str.
// if contains pipe parseBySpace both commands
// if not, parseBySpace the first command
int processString(char *str, char **parsed, char **parsedPipe);
// -*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*-

// -*--*--*--*--*-- EXECUTION OF COMMAND --*--*--*--*--*--*--*-

// handles the parsed command tokens
// if built in executes the command
// if not returns 0
int commandHandler(char **parsed);

// executes general linux commands without pipe
void execute(char **parsed, char *isBackground);

// executes general linux commands with pipe
void executePiped(char **parsed, char **parsedPipe);
// -*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*-

// -*--*--*--*--*--*-- MAIN FUNCTION *--*--*--*--*--*--*--*--*-
int main() {
    char input[MAX_CHAR_COUNT]; // command input
    char *parsed[MAX_ARG_COUNT] = {NULL}; // first command (until "|")
    char *parsedPipe[MAX_ARG_COUNT] = {NULL}; // second command (after "|")
    char *isBackground; // is background command (&)
    char *username = getenv("USER"); // will be used inside printShell()

    startShell();
    while (1) {
        // print "myshell>"
        printShell(username);
        // read command line
        if (getInput(input) != 0) {
            continue;
        }
        // check if contains "&"
        isBackground = strstr(input, "&");
        // parse command
        int isPiped = processString(input, parsed, parsedPipe);
        // try to execute if built in
        if (commandHandler(parsed)) {
            continue;
        }
        if (isPiped) {
            // execute with pipe
            executePiped(parsed, parsedPipe);
        } else {
            execute(parsed, isBackground);
        }

    }
    return 0;
}
// -*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*-

// -*--*--*--*--*--*--*-- HISTORY *--*--*--*--*--*--*--*--*--*-
void enqueueHistory(char *command) {
    for (int i = HISTORY_LIMIT - 1; i > 0; i--) {
        strcpy(history[i], history[i - 1]); // shift every entry to left from last to first
    }
    strcpy(history[0], command); // add the latest
}

void printHistoryQueue() {
    printf(K_WHT);
    for (int i = 0; i < HISTORY_LIMIT && history[i][0] != 0; i++) {
        printf("[%d] %s\n", i + 1, history[i]);
    }
    printf(K_NRM);
}
// -*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*-

// -*--*--*--*- SIMPLE SHELL & BUILT-IN FUNCTIONS --*--*--*--*-
void startShell() {
    printf(K_BLU "Welcome to U-Shel !\nCreated by UMUT YILDIZ - 260201028\nType help for available commands\n" K_NRM);
}

void closeShell() {
    printf(K_MAG "Bye :) \n" K_NRM);
    exit(0);
}

void printHelp() {
    printf(K_BLU
           "***WELCOME TO U SHELL HELP***"
           "\nCreated by UMUT YILDIZ - 260201028"
           "\nList of built-in commands:"
           "\n> cd"
           "\n> dir"
           "\n> bye"
           "\n> exit"
           "\n> help"
           "\n> and general UNIX shell commands"
           "\npipe handling with at most 2 command\n"
           K_NRM);
}

void printShell(char *name) {
    printf(K_YEL "%s", name);
    printf(K_NRM "@");
    printf(K_GRN "U-shell>" K_NRM);
}

void printFailure(char *message) {
    printf(K_RED "%s\n", message);
    printf(K_NRM);
}

void changeDirectory(char **parsed) {
    char path[100];
    if (parsed[1] == NULL || strcmp("~", parsed[1]) == 0) { // if cd called without argument
        chdir(getenv("HOME")); // change directory
        setenv("PWD", getenv("HOME"), 1); // change pwd env value
    } else {
        if (chdir(parsed[1]) == 0) { // if chdir can change directory
            getcwd(path, sizeof(path));
            setenv("PWD", path, 1); // change pwd env value
        } else { // print failure, the path given is probably does not exist
            char message[128];
            snprintf(message, sizeof(message), "cd: The directory '%s' does not exist", parsed[1]);
            printFailure(message);
        }
    }
}

void printDirectory() {
    char buffer[1024];
    getcwd(buffer, sizeof(buffer));
    printf(K_CYN "%s\n", buffer);
    printf(K_NRM);
}

int getInput(char *input) {
    char buffer[MAX_CHAR_COUNT];
    fgets(buffer, MAX_CHAR_COUNT, stdin);
    // if buffer is one letter. This check if user entered empty command.
    // when you tap enter to compute the input, fgets gets the last new line character too
    // there cannot be 0 length input here
    if (strlen(buffer) == 1) {
        return 1;
    }
    char *str = strtok(buffer, "\n");
    strcpy(input, str);
    enqueueHistory(input);
    return 0;
}
// -*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*-

// -*--*--*--*- PREPARE THE ENTERED COMMAND --*--*--*--*--*--*-
int parseByPipe(char *str, char **parsedPipe) {
    parsedPipe[0] = strtok(str, "|"); // first command
    int i = 0;
    while (parsedPipe[i] != NULL) {
        i++;
        parsedPipe[i] = strtok(NULL, "|");// next token (next command)
    }
    return i - 1; // if i = 2 then there has to be pipe
}

void parseBySpace(char *str, char **parsed) {
    parsed[0] = strtok(str, " "); // command keyword
    int i = 0;
    while (parsed[i] != NULL) {
        i++;
        parsed[i] = strtok(NULL, " "); // next token (next argument)
    }
}

int processString(char *str, char **parsed, char **parsedPipe) {
    char *piped[3]; // it is 3 because in order to work with strtok I needed one more entry
    // ex : [first, second, NULL]
    int isPiped = parseByPipe(str, piped);

    if (isPiped) {
        parseBySpace(piped[0], parsed);
        parseBySpace(piped[1], parsedPipe);
    } else {
        parseBySpace(str, parsed);
    }

    return isPiped;
}
// -*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*-

// -*--*--*--*--*-- EXECUTION OF COMMAND --*--*--*--*--*--*--*-
int commandHandler(char **parsed) {
    // known commands which are built in
    char *builtInCommands[] = {"cd", "dir", "history", "bye", "exit", "help"};
    int builtInCommandCount = sizeof(builtInCommands) / sizeof(builtInCommands[0]);
    int command = -1; // will be used in switch

    // loop trough builtInCommands until find same
    for (int i = 0; i < builtInCommandCount; i++) {
        if (strcasecmp(parsed[0], builtInCommands[i]) == 0) {
            command = i;
            break;
        }
    }

    switch (command) {
        case 0: // "cd"
            changeDirectory(parsed);
            break;
        case 1: // "dir"
            printDirectory();
            break;
        case 2: // "history"
            printHistoryQueue();
            break;
        case 3: // "bye"
        case 4: // "exit"
            closeShell();
            break;
        case 5: // "help"
            printHelp();
            break;
        default:
            break;
    }

    return command != -1 ? 1 : 0; // if command not found in builtInCommands return 1
}

void execute(char **parsed, char *isBackground) {
    pid_t pID = fork();
    if (pID == -1) {
        printFailure("FAILED FORK");
        return;
    } else if (pID == 0) {
        // child process
        if (execvp(parsed[0], parsed) < 0) {
            // print the failure
            char message[128];
            snprintf(message, sizeof(message), "U-SHELL: Unknown command: `%s`", parsed[0]);
            printFailure(message);
        }
        exit(0);
    } else {
        if (!isBackground) {
            // foreground commands need to wait
            wait(NULL);
        }
        return;
    }
}

void executePiped(char **parsed, char **parsedPipe) {
    int pipe_fd[2];
    pid_t pID1, pID2;
    // establish the pipe
    if (pipe(pipe_fd) < 0) {
        printFailure("\nPipe could not be initialized");
        return;
    }
    pID1 = fork();
    if (pID1 < 0) {
        printFailure("FAILED FORK");
        return;
    } else if (pID1 == 0) {
        // first child process
        close(pipe_fd[READ_END]); // close read_end
        dup2(pipe_fd[WRITE_END], STDOUT_FILENO); // dup the write_end. With this duping, process will send message
        close(pipe_fd[WRITE_END]); // then close write_end

        if (execvp(parsed[0], parsed) < 0) {
            // print failure of first child
            char message[128];
            snprintf(message, sizeof(message), "U-SHELL: Unknown command: `%s`", parsed[0]);
            printFailure(message);
            exit(0);
        }

    } else {
        pID2 = fork();
        if (pID2 < 0) {
            printFailure("FAILED FORK");
            return;
        } else if (pID2 == 0) {
            // second child process
            close(pipe_fd[WRITE_END]); // close write_end
            dup2(pipe_fd[READ_END], STDIN_FILENO); // dup the read_end. With this duping, process will read message
            close(pipe_fd[READ_END]); // then close read_end
            if (execvp(parsedPipe[0], parsedPipe) < 0) {
                // print failure
                char message[128];
                snprintf(message, sizeof(message), "U-SHELL: Unknown command: `%s`", parsedPipe[0]);
                printFailure(message);
                exit(0);
            }
        } else {
            // close pipe and wait both
            close(pipe_fd[READ_END]);
            close(pipe_fd[WRITE_END]);
            wait(NULL);
            wait(NULL);
        }
    }
}

// -*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*-