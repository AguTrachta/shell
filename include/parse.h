#ifndef PARSE_H
#define PARSE_H

/**
 * @brief Splits a command into subcommands separated by pipes ('|').
 *
 * This function takes a string containing a command with one or more pipes ('|')
 * and splits it into individual subcommands. Each subcommand is trimmed of
 * leading and trailing whitespace.
 *
 * @param command The string containing the full command, which may include
 *        one or more pipes.
 * @param num_commands A pointer to an integer used to store the number
 *        of subcommands found.
 * @return An array of strings (`char**`) where each element is a subcommand.
 *         The last element of the array is `NULL` to indicate the end.
 *         The function returns `NULL` if a memory allocation error occurs.
 */
char** split_by_pipes(char* command, int* num_commands);

/**
 * @brief Splits a command into tokens and handles input and output redirection.
 *
 * This function takes a command and splits it into individual arguments (tokens),
 * while also parsing input and output redirection if present, using the `<` and `>`
 * operators. The redirection files found are stored in the pointers `input_file_ptr`
 * and `output_file_ptr`.
 *
 * @param command The string containing the full command.
 * @param input_file_ptr A `char*` pointer where the input redirection filename
 *        will be stored if present; otherwise, it is left as `NULL`.
 * @param output_file_ptr A `char*` pointer where the output redirection filename
 *        will be stored if present; otherwise, it is left as `NULL`.
 * @return An array of strings (`char**`) where each element is a token of the command.
 *         The last element of the array is `NULL` to indicate the end.
 *         The function returns `NULL` if a memory allocation error occurs.
 */
char** parse_command(char* command, char** input_file_ptr, char** output_file_ptr);

#endif // PARSE_H
