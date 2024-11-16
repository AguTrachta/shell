#ifndef SHELL_H
#define SHELL_H

#include <signal.h>
#include <stdio.h>
#include <sys/types.h>

/**
 * @brief The maximum number of file descriptors used for signal pipes.
 */
#define MAX_FS 2

/**
 * @brief File descriptors for signal pipes.
 */
extern int sig_pipe_fds[MAX_FS];

/**
 * @brief Process ID of the foreground job.
 */
extern pid_t foreground_pid;

/**
 * @brief Flag indicating if a SIGCHLD signal was received.
 */
extern volatile sig_atomic_t sigchld_flag;

/**
 * @brief Initializes the shell environment, setting up signal handlers and preparing the shell for execution.
 */
void init_shell();

/**
 * @brief Displays the command-line prompt in the specified format.
 *
 * The prompt includes the current user, hostname, and working directory.
 *
 * @return A string representing the formatted prompt.
 */
char* display_prompt();

/**
 * @brief Reads a line of command input from the provided input stream.
 *
 * This function reads a line from the given input stream (stdin or a file).
 * It handles interruptions from signals and properly clears the error state
 * if the read operation is interrupted.
 *
 * @param input_stream The input stream (either stdin or a file).
 * @return A string containing the command line input. Returns NULL if EOF is reached.
 */
char* read_command(FILE* input_stream);

/**
 * @brief Executes the specified command.
 *
 * This function handles both piped and single commands, splitting them as necessary
 * and executing them either directly or through a chain of pipes.
 *
 * @param command The command line to execute.
 * @return Always returns 1 to continue shell execution.
 */
int execute_command(char* command);

/**
 * @brief Executes commands from a batch file.
 *
 * This function reads and executes commands from a specified batch file. It ignores
 * empty lines and comments. The shell exits if a command returns 0.
 *
 * @param batch_file The file containing the commands to execute.
 * @return Returns 1 to continue shell execution or 0 to exit.
 */
int execute_batch_file(FILE* batch_file);

/**
 * @brief Cleans up resources used by the shell before exiting.
 *
 * This function frees allocated memory for background jobs and performs
 * any necessary cleanup operations.
 */
void cleanup_shell();

/**
 * @brief Structure representing a background job.
 */
typedef struct job
{
    int job_id;       /**< Unique ID assigned to the job */
    pid_t pid;        /**< Process ID of the job */
    char* command;    /**< Command associated with the job */
    struct job* next; /**< Pointer to the next job in the list */
} job_t;

/**
 * @brief Adds a job to the background job list.
 *
 * This function creates a new job with the specified process ID and command,
 * assigns it a unique job ID, and adds it to the job list.
 *
 * @param pid The process ID of the job.
 * @param command The command executed by the job.
 * @return The unique job ID assigned to the job.
 */
int add_job(pid_t pid, const char* command);

/**
 * @brief Removes a job from the background job list.
 *
 * This function searches the job list for a job with the specified process ID,
 * removes it from the list, and frees associated memory.
 *
 * @param pid The process ID of the job to remove.
 */
void remove_job(pid_t pid);

/**
 * @brief Handles the logic for the SIGCHLD signal.
 *
 * This function is called when a SIGCHLD signal is received, indicating that
 * a child process has terminated. It removes the corresponding job from the
 * job list and handles the child process's status.
 */
void sigchld_handler_logic();

/**
 * @brief Animates the startup sequence of the shell.
 *
 * This function simulates a progressive loading sequence with delays, displaying
 * system initialization messages to enhance user experience.
 */
void animate_startup();

#endif // SHELL_H
