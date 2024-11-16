#ifndef COMMANDS_H
#define COMMANDS_H

#include "shell.h"
#include <stdio.h>

/**
 * @brief Executes the built-in 'cd' command to change the current directory.
 *
 * @param args Array of arguments. args[0] is "cd", args[1] is the target
 * directory.
 * @return 1 to continue shell execution, 0 to exit if desired.
 */
int cmd_cd(char **args);

/**
 * @brief Executes the built-in 'clr' command to clear the screen.
 *
 * @return 1 to continue shell execution.
 */
int cmd_clr();

/**
 * @brief Executes the built-in 'echo' command to display messages or
 * environment variables.
 *
 * @param args Array of arguments. args[0] is "echo", args[1...] are the
 * comments or variables to display.
 * @return 1 to continue shell execution.
 */
int cmd_echo(char **args);

/**
 * @brief Executes the built-in 'quit' command to exit the shell.
 *
 * @return 0 to indicate that the shell should terminate.
 */
int cmd_quit();

/**
 * @brief Executes the 'help' command to display available internal commands and
 * their usage.
 *
 * @return 1 to continue shell execution.
 */
int cmd_help();

/**
 * @brief Starts the monitoring process, managing the monitoring process
 * lifecycle.
 *
 * This function checks if a monitoring process is already running and, if so,
 * stops it before starting a new one.
 *
 * @return 1 to continue shell execution.
 */
int cmd_start_monitor();

/**
 * @brief Stops the active monitoring process.
 *
 * This function reads the process ID of the monitoring process from a PID file
 * and sends a termination signal.
 *
 * @return 1 to continue shell execution.
 */
int cmd_stop_monitor();

/**
 * @brief Displays the status of the monitoring system based on the provided
 * option.
 *
 * This command can display various system metrics, such as CPU usage, memory
 * usage, disk statistics, and network statistics. Optionally, the user can
 * specify an option to filter the displayed metrics.
 *
 * @param option A string indicating the specific metric to display.
 * @return 1 to continue shell execution.
 */
int cmd_status_monitor(const char *option);

int cmd_searchconfig(char **args);

/**
 * @brief Checks if a command is an internal command and executes it if
 * applicable.
 *
 * This function verifies if a given command is an internal command (such as
 * 'cd', 'echo', 'quit') and, if so, executes it. It handles input and output
 * redirection as well as background execution for these commands.
 *
 * @param args Array of arguments. args[0] is the command.
 * @param input_file File for input redirection (can be NULL).
 * @param output_file File for output redirection (can be NULL).
 * @param background Flag indicating if the command should run in the
 * background.
 * @return 1 to continue shell execution, 0 to exit if 'quit' is executed, -1 if
 * the command is not internal.
 */
int execute_internal_command(char **args, char *input_file, char *output_file,
                             int background);

/**
 * @brief Executes an external command by forking a new process.
 *
 * This function handles input/output redirection and background execution for
 * external commands.
 *
 * @param args Array of arguments for the command.
 * @param background Flag indicating if the command should run in the
 * background.
 * @param command_copy A copy of the original command string.
 * @param input_file File for input redirection (can be NULL).
 * @param output_file File for output redirection (can be NULL).
 * @return 1 to continue shell execution.
 */
int execute_external_command(char **args, int background, char *command_copy,
                             char *input_file, char *output_file);

/**
 * @brief Executes a single command.
 *
 * This function parses a single command, checks if it's an internal command,
 * and if not, runs it as an external command.
 *
 * @param command The command string to execute.
 * @return The status code of the command execution.
 */
int execute_single_command(char *command);

/**
 * @brief Executes multiple commands connected by pipes.
 *
 * This function creates a chain of processes, each connected by pipes, to
 * execute multiple commands.
 *
 * @param commands Array of command strings to execute in a pipeline.
 * @param num_commands The number of commands in the pipeline.
 * @return 1 to continue shell execution.
 */
int execute_piped_commands(char **commands, int num_commands);

void search_directory_recursive(const char *directory, const char *extension);
int has_extension(const char *filename, const char *extension);
void print_file_content(const char *filepath);

#endif // COMMANDS_H
