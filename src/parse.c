#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUFSIZE 64  // Initial buffer size for token arrays
#define BUFFER_INCREMENT 64 // Amount by which to expand buffer when needed

static char* input_file = NULL;
static char* output_file = NULL;

char** split_by_pipes(char* command, int* num_commands)
{
    int bufsize = INITIAL_BUFSIZE;                   // Initial buffer size to handle a moderate number of subcommands
    int position = 0;                                // Tracks the current position in the tokens array
    char** tokens = malloc(bufsize * sizeof(char*)); // Array to store the subcommands
    char* token;                                     // Pointer to hold each subcommand while processing pipes

    if (!tokens)
    {
        fprintf(stderr, "Shell: memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    // Use strtok to split by '|'
    token = strtok(command, "|");
    while (token != NULL)
    {
        // Remove leading whitespace
        while (*token == ' ')
            token++;
        char* end = token + strlen(token) - 1; // Points to the last character of the subcommand

        // Remove trailing whitespace or tabs
        while (end > token && (*end == ' ' || *end == '\t'))
            *end-- = '\0';

        // Copy the token to store independently of the original memory
        tokens[position++] = strdup(token);

        // Expand the tokens array if necessary
        if (position >= bufsize)
        {
            bufsize += BUFFER_INCREMENT;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens)
            {
                fprintf(stderr, "Shell: memory allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        // Continue tokenizing by '|'
        token = strtok(NULL, "|");
    }
    tokens[position] = NULL;  // Mark end of the array
    *num_commands = position; // Store the total number of subcommands
    return tokens;            // Contains each subcommand separated by pipes
}

/* Handle input and output redirection and treat a sequence of words as a single argument */
char** parse_command(char* command, char** input_file_ptr, char** output_file_ptr)
{
    int bufsize = INITIAL_BUFSIZE;
    int position = 0;
    char** tokens = malloc(bufsize * sizeof(char*));
    char* cmd_copy = strdup(command);
    char* ptr = cmd_copy;

    if (!tokens)
    {
        fprintf(stderr, "Shell: memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    *input_file_ptr = NULL;
    *output_file_ptr = NULL;

    while (*ptr)
    {
        // Skip whitespace
        while (*ptr == ' ' || *ptr == '\t')
            ptr++;

        if (*ptr == '\0')
            break;

        // Handle input/output redirection
        if (*ptr == '<' || *ptr == '>')
        {
            char redir_op = *ptr;
            ptr++;
            // Skip whitespace
            while (*ptr == ' ' || *ptr == '\t')
                ptr++;
            // Capture the filename
            char* start = ptr;
            while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '<' && *ptr != '>')
                ptr++;
            int len = ptr - start;
            char* filename = strndup(start, len);

            if (redir_op == '<')
                *input_file_ptr = filename;
            else
                *output_file_ptr = filename;
        }
        // Handle quoted arguments
        else
        {
            char* start = ptr;

            // Handle single quotes
            if (*ptr == '\'')
            {
                ptr++;
                start = ptr;
                while (*ptr && *ptr != '\'')
                    ptr++;
                if (*ptr == '\'')
                {
                    *ptr = '\0';
                    ptr++;
                }
            }
            // Handle double quotes
            else if (*ptr == '\"')
            {
                ptr++;
                start = ptr;
                while (*ptr && *ptr != '\"')
                    ptr++;
                if (*ptr == '\"')
                {
                    *ptr = '\0';
                    ptr++;
                }
            }
            else
            {
                while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '<' && *ptr != '>')
                    ptr++;
            }

            // Add token to the array
            if (*ptr == ' ' || *ptr == '\t' || *ptr == '<' || *ptr == '>' || *ptr == '\0')
            {
                int len = ptr - start;
                char* token = strndup(start, len);
                tokens[position++] = token;

                if (position >= bufsize)
                {
                    bufsize += BUFFER_INCREMENT;
                    tokens = realloc(tokens, bufsize * sizeof(char*));
                    if (!tokens)
                    {
                        fprintf(stderr, "Shell: memory allocation error\n");
                        exit(EXIT_FAILURE);
                    }
                }

                if (*ptr == '<' || *ptr == '>')
                    continue;
                else if (*ptr != '\0')
                    ptr++;
            }
        }
    }

    tokens[position] = NULL;
    free(cmd_copy);
    return tokens;
}
